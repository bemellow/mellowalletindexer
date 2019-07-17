using System;
using System.Collections.Generic;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using EthereumClasses;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace EthereumIndex
{
    class RestService : IDisposable
    {
        private static readonly log4net.ILog Log = log4net.LogManager.GetLogger(MethodBase.GetCurrentMethod().DeclaringType);

        //private static Regex _urlRegex = new Regex(@"/([a-z]+)/(.*)");
        private HttpListener _listener = new HttpListener();
        private delegate void RequestHandler(HttpListenerResponse response, HttpListenerRequest request, string requestBody);
        private Dictionary<string, RequestHandler> _handlers = new Dictionary<string, RequestHandler>();
        private EthIndex _index;

        private void AddEndpoint(string method, string path, RequestHandler handler)
        {
            _handlers.Add(method.ToUpper() + "+" + path.ToLower(), handler);
        }

        public RestService(EthIndex index)
        {
            _index = index;
            _listener.Prefixes.Add($"http://*:{Configuration.Get().RestPort}/");
            _listener.Start();

            AddEndpoint("POST", "/api/history", HandleHistory);
            AddEndpoint("POST", "/api/gasprice", HandleGasPrice);

            foreach (var token in index.Tokens)
                AddEndpoint("POST", $"/api/{token.Address}/history", (response, request, body) => HandleTokenHistory(token, response, request, body));
        }

        public void Dispose()
        {
            if (_listener != null)
            {
                _listener.Stop();
                _listener.Close();
                _listener = null;
            }
        }

        private static string BufferToString(byte[] input)
        {
            var ret = Convert.ToBase64String(input);
            ret = ret.Replace('+', '-').Replace('/', '_').Replace('=', '!');
            return ret;
        }

        private static byte[] StringToBuffer(string input)
        {
            input = input.Replace('!', '=').Replace('_', '/').Replace('-', '+');
            return Convert.FromBase64String(input);
        }

        public void Run(Func<bool> continueRunning)
        {
            Log.Info("REST service running.");
            Thread.CurrentThread.Name = "EthereumIndex REST";
            while (true)
            {
                try
                {
                    var async = _listener.BeginGetContext(HandleAsyncRequest, _listener);
                    Log.Info("Ready to handle new request.");
                    bool run;
                    while ((run = continueRunning()) && !async.AsyncWaitHandle.WaitOne(1000)) ;
                    if (!run)
                        break;
                }
                catch (SocketException)
                {
                    break;
                }
                catch
                {
                }
            }
        }

        private void HandleAsyncRequest(IAsyncResult result)
        {
            try
            {
                HandleRequest(((HttpListener) result.AsyncState).EndGetContext(result));
            }
            catch (ObjectDisposedException)
            {
            }
            catch (OperationAbortedException)
            {
            }
        }

        private void HandleRequest(HttpListenerContext context)
        {
            try
            {
                var url = context.Request.RawUrl;
                var body = context.Request.GetBody();
                //Log.Info($"Request URL: {url}");
                //Log.Info($"Request headers: {context.Request.Headers}");
                //Log.Info($"Request method: {context.Request.HttpMethod}");
                //Log.Info($"Request body: {body}");

                var key = context.Request.HttpMethod.ToUpper() + "+" + url.ToLower();
                Log.Info($"Handling {key}");
                RequestHandler handler;
                if (!_handlers.TryGetValue(key, out handler))
                {
                    Log.Error($"Couldn't find handler for request {key}");
                    context.Response.WriteString(string.Empty);
                    return;
                }
                var sw = new Stopwatch();
                sw.Start();
                handler(context.Response, context.Request, body);
                sw.Stop();
                Log.Info($"{key} handled in {sw.ElapsedMilliseconds} ms.");
            }
            catch (Exception e)
            {
                Console.Error.WriteLine(e.Message);
                Console.Error.WriteLine(e.StackTrace);
                context.Response.WriteString($"<h1>Error: {e.Message}</h1>");
            }
        }

        //void HandleUtxo(HttpListenerResponse response, HttpListenerRequest request, string body)
        //{
        //    response.WriteString(_index.GetUtxo(body));
        //}
        //
        //void HandleBalance(HttpListenerResponse response, HttpListenerRequest request, string body)
        //{
        //    response.WriteString(_index.GetBalance(body));
        //}
        //
        //void HandleBalances(HttpListenerResponse response, HttpListenerRequest request, string body)
        //{
        //    response.WriteString(_index.GetBalances(body));
        //}
        //
        void HandleHistory(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            var p = JsonConvert.DeserializeObject<HistoryRequestParams>(body);
            var history = _index.GetHistory(p.Addresses, p.MaxTxs, p.Ascending != 0);
            response.WriteString(JsonConvert.SerializeObject(history));
        }

        void HandleGasPrice(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            var gasPrice = _index.GetGasPrice();
            response.WriteString(JsonConvert.SerializeObject(gasPrice.ToString()));
        }

        void HandleTokenHistory(Token token, HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            var p = JsonConvert.DeserializeObject<HistoryRequestParams>(body);
            var history = _index.GetHistory(token, p.Addresses, p.MaxTxs, p.Ascending != 0);
            response.WriteString(JsonConvert.SerializeObject(history));
        }
    }
}
