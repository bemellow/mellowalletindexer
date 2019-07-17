#define CUSTOM_SOCKET
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using Newtonsoft.Json.Linq;
using Org.BouncyCastle.Math;

namespace EthereumClasses.InternalCommunication
{
#if CUSTOM_SOCKET
    public class LineBasedSocket
    {
        private Socket _socket;
        private string _host;
        private int _port;
        private LinkedList<ArraySegment<byte>> _buffers = new LinkedList<ArraySegment<byte>>();

        public LineBasedSocket(string host, int port)
        {
            _host = host;
            _port = port;
            _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            _socket.Connect(host, port);
        }

        private void ReadMore()
        {
            var buffer = new byte[4096];
            var read = _socket.Receive(buffer, 0, buffer.Length, SocketFlags.None);
            if (read == 0)
                return;
            _buffers.AddLast(new ArraySegment<byte>(buffer, 0, read));
        }

        public byte ReadByte()
        {
            if (_buffers.Count == 0)
            {
                ReadMore();
                if (_buffers.Count == 0)
                    throw new Exception("No more data.");
            }
            var segment = _buffers.First.Value;
            _buffers.RemoveFirst();
            var ret = segment.Array[segment.Offset];
            if (segment.Count != 1)
            {
                _buffers.AddFirst(new ArraySegment<byte>(segment.Array, segment.Offset + 1, segment.Count - 1));
            }
            return ret;
        }

        public string ReadLine()
        {
            var accum = new List<byte>();
            bool cr = false;
            while (true)
            {
                var b = ReadByte();
                var oldCr = cr;
                cr = b == '\r';
                if (oldCr)
                {
                    var ret = Encoding.UTF8.GetString(accum.ToArray());
                    if (!cr && b != '\n')
                        _buffers.AddFirst(new ArraySegment<byte>(new byte[] { b }, 0, 1));
                    return ret;
                }

                if (cr)
                    continue;
                if (b == '\n')
                    return Encoding.UTF8.GetString(accum.ToArray());
                accum.Add(b);
            }
        }

        public byte[] ReadNBytes(long n)
        {
            var ret = new byte[n];
            long offset = 0;
            while (n > 0)
            {
                if (_buffers.Count == 0)
                {
                    ReadMore();
                    if (_buffers.Count == 0)
                        throw new Exception("No more data.");
                    continue;
                }
                var segment = _buffers.First.Value;
                _buffers.RemoveFirst();
                var readSize = (int)Math.Min(segment.Count, n);
                Array.Copy(segment.Array, segment.Offset, ret, offset, readSize);
                if (readSize != segment.Count)
                    _buffers.AddFirst(new ArraySegment<byte>(segment.Array, segment.Offset + readSize, segment.Count - readSize));
                offset += readSize;
                n -= readSize;
            }
            return ret;
        }

        public void Write(byte[] buffer)
        {
            _socket.Send(buffer);
        }
    }

    public class HttpSocket
    {
        private LineBasedSocket _socket;
        private string _host;
        private int _port;

        public HttpSocket(string host, int port)
        {
            _host = host;
            _port = port;
            _socket = new LineBasedSocket(host, port);
        }

        public string SendPutRequest(string request)
        {
            if (!request.All(x => x < 0x80))
                throw new Exception("Invalid characters in request.");
            _socket.Write(Encoding.UTF8.GetBytes(
                "PUT / HTTP/1.1\r\n" +
                $"Host: {_host}\r\n" +
                "Content-Type: application/json\r\n" +
                $"Content-Length: {request.Length}\r\n" +
                "Connection: keep-alive\r\n" +
                "\r\n" + request));

            var unused = _socket.ReadLine();
            var statusString = _socket.ReadLine();
            var split = statusString.Split(new[] { ' ' });
            if (split.Length < 2)
                return null;
            int status;
            if (!int.TryParse(split[1], out status) || status != 200)
                return null;
            var headers = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
            while (true)
            {
                var line = _socket.ReadLine();
                if (line.Length == 0)
                    break;
                var colon = line.IndexOf(':');
                var key = line.Substring(0, colon);
                var value = line.Substring(colon + 1).TrimStart();
                headers[key] = value;
            }

            string contentLengthString;
            if (headers.TryGetValue("content-length", out contentLengthString))
            {
                var contentLength = Convert.ToInt64(contentLengthString);
                return Encoding.UTF8.GetString(_socket.ReadNBytes(contentLength));
            }

            //Chunked mode:

            var accum = new List<byte>();
            while (true)
            {
                var lengthString = _socket.ReadLine();
                var length = long.Parse(lengthString, System.Globalization.NumberStyles.HexNumber);
                if (length == 0)
                {
                    _socket.ReadLine();
                    return Encoding.UTF8.GetString(accum.ToArray());
                }
                accum.AddRange(_socket.ReadNBytes(length));
                _socket.ReadLine();
            }
        }
    }
#endif

    public class GethRequester
    {
#if CUSTOM_SOCKET
        //private string _lightweightRequestString;
        //private string _fullRequestString;
        private string _url;
        private string _host;
        private HttpSocket _socket;

        public GethRequester(string host = "localhost")
        {
            _host = host;
            _url = $"http://{_host}:8545/";
            _socket = new HttpSocket(host, 8545);
            //_fullRequestString = new StreamReader("full_request.js").ReadToEnd();
            //using (var file = new StreamReader("light_request.js"))
            //    _lightweightRequestString = file.ReadToEnd();
            //_fullRequestString = FilterWhitespace(_fullRequestString);
            //_lightweightRequestString = FilterWhitespace(_lightweightRequestString);
        }

        private static string FilterWhitespace(string s)
        {
            var ret = new StringBuilder();
            foreach (var c in s)
            {
                if (char.IsWhiteSpace(c))
                    continue;
                ret.Append(c);
            }
            return ret.ToString();
        }

        /*private string BuildRequest(string tx, bool lightweight)
        {
            var sb = new StringBuilder();
            sb.Append("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"debug_traceTransaction\",\"params\":[\"");
            sb.Append(tx);
            sb.Append("\",");
            sb.Append(lightweight ? _lightweightRequestString : _fullRequestString);
            sb.Append("]}");
            var ret = sb.ToString();
            return ret;
        }*/

        /*private List<ParsedCall> RequestTrace(string transactionHash, bool lightweight)
        {
            var requestBody = BuildRequest(transactionHash, lightweight);
            var s = _socket.SendPutRequest(requestBody);
            if (s == null)
                return null;
            using (var reader = new StringReader(s))
            {
                return InstructionParser2.Parse(reader);
            }
        }*/

        public BigInteger GetBlockNumber()
        {
            const string request = "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\"}";
            var s = _socket.SendPutRequest(request);
            if (s == null)
                return null;
            string hex;
            try
            {
                var jobj = JObject.Parse(s);
                hex = jobj["result"].Value<string>();
            }
            catch
            {
                return null;
            }
            try
            {
                return Utility.HexStringToBigInteger(hex);
            }
            catch
            {
                return null;
            }
        }

        private JObject GetTransactionJsonByHash(string hash)
        {
            string request = $"{{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionByHash\",\"params\":[\"{hash}\"]}}";
            var s = _socket.SendPutRequest(request);
            if (s == null)
                return null;
            try
            {
                return JObject.Parse(s)["result"] as JObject;
            }
            catch
            {
                return null;
            }
        }

        private static Transaction ParseTransaction(JObject json)
        {
            try
            {
                return new Transaction(json);
            }
            catch
            {
                return null;
            }
        }

        public Transaction GetTransactionByHash(string hash)
        {
            var json = GetTransactionJsonByHash(hash);
            if (json == null)
                return null;
            return ParseTransaction(json);
        }

        private JObject GetBlockWithTransactionsJsonByNumber(ulong blockNo)
        {
            var hex = blockNo.ToString("x");
            string request = $"{{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getBlockByNumber\",\"params\":[\"0x{hex}\",true]}}";
            var s = _socket.SendPutRequest(request);
            if (s == null)
                return null;
            try
            {
                return JObject.Parse(s)["result"] as JObject;
            }
            catch
            {
                return null;
            }
        }

        private static BlockWithTransactions ParseBlockWithTransactions(JObject json)
        {
            try
            {
                return new BlockWithTransactions(json);
            }
            catch
            {
                return null;
            }
        }

        public BlockWithTransactions GetBlockWithTransactions(ulong blockNo)
        {
            var json = GetBlockWithTransactionsJsonByNumber(blockNo);
            if (json == null)
                return null;
            return ParseBlockWithTransactions(json);
        }

        private string GetCodeJson(string address, ulong blockNo)
        {
            var hex = blockNo.ToString("x");
            string request = $@"{{""id"":1,""jsonrpc"":""2.0"",""method"":""eth_getCode"",""params"":[""{address}"",""0x{hex}""]}}";
            var s = _socket.SendPutRequest(request);
            if (s == null)
                return null;
            try
            {
                return JObject.Parse(s)["result"].Value<string>();
            }
            catch
            {
                return null;
            }
        }

        private static byte[] ParseCode(string hex)
        {
            try
            {
                return Utility.ParseHexString(hex).ToArray();
            }
            catch
            {
                return null;
            }
        }

        public byte[] GetCode(string address, ulong blockNo)
        {
            var s = GetCodeJson(address, blockNo);
            if (s == null)
                return null;
            return ParseCode(s);
        }

        private JObject GetTransactionReceiptJson(string transactionHash)
        {
            var request = $@"{{""id"":1,""jsonrpc"":""2.0"",""method"":""eth_getTransactionReceipt"",""params"":[""{transactionHash}""]}}";
            var s = _socket.SendPutRequest(request);
            if (s == null)
                return null;
            try
            {
                return JObject.Parse(s)["result"] as JObject;
            }
            catch
            {
                return null;
            }
        }

        private static TransactionReceipt ParseTransactionReceipt(JObject json)
        {
            try
            {
                return new TransactionReceipt(json);
            }
            catch
            {
                return null;
            }
        }

        public TransactionReceipt GetTransactionReceipt(string transactionHash)
        {
            var json = GetTransactionReceiptJson(transactionHash);
            if (json == null)
                return null;
            return ParseTransactionReceipt(json);
        }
#else
        private string _lightweightRequestString;
        private string _fullRequestString;
        private string _host;

        public GethRequester(string host = "localhost")
        {
            _host = $"http://{host}:8545/";
            _fullRequestString = new StreamReader("full_request.js").ReadToEnd();
            _lightweightRequestString = new StreamReader("light_request.js").ReadToEnd();
            _fullRequestString = FilterWhitespace(_fullRequestString);
            _lightweightRequestString = FilterWhitespace(_lightweightRequestString);
        }

        private static string FilterWhitespace(string s)
        {
            var ret = new StringBuilder();
            foreach (var c in s)
            {
                if (char.IsWhiteSpace(c))
                    continue;
                ret.Append(c);
            }
            return ret.ToString();
        }

        private string BuildRequest(string tx, bool lightweight)
        {
            var sb = new StringBuilder();
            sb.Append("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"debug_traceTransaction\",\"params\":[\"");
            sb.Append(tx);
            sb.Append("\",");
            sb.Append(lightweight ? _lightweightRequestString : _fullRequestString);
            sb.Append("]}");
            var ret = sb.ToString();
            return ret;
        }

        private List<ParsedCall> RequestTrace(string transactionHash, bool lightweight)
        {
            var encoded = Encoding.UTF8.GetBytes(BuildRequest(transactionHash, lightweight));
            WebRequest request = WebRequest.Create(_host);
            request.Method = "PUT";
            request.ContentType = "application/json";
            request.ContentLength = encoded.Length;
            using (var stream = request.GetRequestStream())
            {
                stream.Write(encoded, 0, encoded.Length);
            }
            using (var response = request.GetResponse() as HttpWebResponse)
            {
                if (response == null || response.StatusCode != HttpStatusCode.OK)
                    return null;

                using (var stream = response.GetResponseStream())
                {
                    if (stream == null)
                        return null;
                    List<ParsedCall> ret;
                    using (var reader = new StreamReader(stream, Encoding.UTF8, false, 4096, true))
                    {
                        ret = InstructionParser2.Parse(reader);
                    }

                    var buffer = new byte[4096];
                    while (true)
                    {
                        if (stream.Read(buffer, 0, buffer.Length) == 0)
                            break;
                    }
                    return ret;
                }
            }
        }
#endif

        /*public List<ParsedCall> RequestTrace(string transactionHash)
        {
            return RequestTrace(transactionHash, false);
        }*/

        /*public object RequestLightweightTrace(string transactionHash)
        {
            return RequestTrace(transactionHash, true);
        }*/
    }
}
