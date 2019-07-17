using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace NktBtcIndex
{
    class RestService : IDisposable
    {
        private static readonly log4net.ILog Log = log4net.LogManager.GetLogger(MethodBase.GetCurrentMethod().DeclaringType);

        private static Regex _urlRegex = new Regex(@"/([a-z]+)/(.*)");
        private HttpListener _listener = new HttpListener();
        private delegate void RequestHandler(HttpListenerResponse response, HttpListenerRequest request, string requestBody);
        private Dictionary<string, RequestHandler> _handlers = new Dictionary<string, RequestHandler>();
        private BtcIndex _index;

        private void AddEndpoint(string method, string path, RequestHandler handler)
        {
            _handlers.Add(method.ToUpper() + "+" + path, handler);
        }

        public RestService(BtcIndex index)
        {
            _index = index;
            _listener.Prefixes.Add($"http://*:{Configuration.Get().RestPort}/");
            _listener.Start();

            AddEndpoint("POST", "/api/utxo", HandleUtxo);
            AddEndpoint("POST", "/api/balance", HandleBalance);
            AddEndpoint("POST", "/api/balances", HandleBalances);
            AddEndpoint("POST", "/api/history", HandleHistory);
            AddEndpoint("GET", "/api/fees", HandleFees);
            AddEndpoint("POST", "/web/utxo", HandleWebUtxo);
            AddEndpoint("GET", "/", HandleIndex);
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
            Thread.CurrentThread.Name = "NktBtcIndex REST";
            while (true)
            {
                try
                {
                    var async = _listener.BeginGetContext(HandleAsyncRequest, _listener);
                    bool run;
                    while ((run = continueRunning()) && !async.AsyncWaitHandle.WaitOne(1000));
                    if (!run)
                        break;
                }
                catch (SocketException e)
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

                var key = context.Request.HttpMethod.ToUpper() + "+" + url;
                Log.Info($"Handling {key}");
                var sw = new Stopwatch();
                sw.Start();
                RequestHandler handler;
                if (!_handlers.TryGetValue(key, out handler))
                {
                    Log.Error($"Couldn't find handler for request {key}");
                    context.Response.WriteString(string.Empty);
                    return;
                }
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

        void HandleUtxo(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            response.WriteString(_index.GetUtxoInsight(body));
        }

        void HandleBalance(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            response.WriteString(_index.GetBalance(body));
        }

        void HandleBalances(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            response.WriteString(_index.GetBalances(body));
        }

        void HandleHistory(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            response.WriteString(_index.GetHistory(body));
        }

        void HandleFees(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            response.WriteString(_index.GetFees());
        }

        private void HandleIndex(HttpListenerResponse response, HttpListenerRequest request, string requestbody)
        {
            const string body = @"
<html>
<head>
<title>NktBtcIndex</title>
</head>
<body>

<h1>NktBtcIndex</h1>

<form action=""/web/utxo"" method=""post"">
Addresses (comma-separated): <input type=""text"" name=""addresses""></input><br/>
<input type=""submit"" name=""submit_someinput"" value=""Submit""></input>
</form>

<hr/>

</body>
</html>
";
            response.WriteString(body);
        }

        void HandleWebUtxo(HttpListenerResponse response, HttpListenerRequest request, string body)
        {
            var requestParameters = Utility.ParseRequestBody(body);
            string addressesString;
            if (!requestParameters.TryGetValue("addresses", out addressesString))
            {
                response.WriteString("{}");
                return;
            }

            var json = new JArray();
            foreach (var address in addressesString.Split(Utility.Comma, StringSplitOptions.RemoveEmptyEntries))
                json.Add(address);

            var result =
                JsonConvert.DeserializeObject<Dictionary<string, List<UtxoResult>>>(
                    _index.GetUtxo(json.ToString(Formatting.None)));

            var sb = new StringBuilder();

            sb.Append(@"<html>
<head>
<title>Utxo</title>
</head>
<body>");

            response.WriteString(_index.GetUtxo(json.ToString(Formatting.None)));
        }
        
    }
}
