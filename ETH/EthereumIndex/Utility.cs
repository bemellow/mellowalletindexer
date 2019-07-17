using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using EthereumClasses;

namespace EthereumIndex
{
    static class Utility
    {
        public static string GetBody(this HttpListenerRequest request)
        {
            return new StreamReader(request.InputStream).ReadToEnd();
        }

        public static void WriteString(this HttpListenerResponse response, string s)
        {
            var buffer = Encoding.UTF8.GetBytes(s);
            response.ContentLength64 = buffer.Length;
            response.OutputStream.Write(buffer, 0, buffer.Length);
            response.OutputStream.Close();
        }

        private static char[] Ampersand = { '&' };
        public static char[] Comma = { ',' };
        private static char[] EqualsChar = { '=' };

        public static Dictionary<string, string> ParseRequestBody(string body)
        {
            var ret = new Dictionary<string, string>();
            var keyValues = body.Split(Ampersand, StringSplitOptions.RemoveEmptyEntries);
            foreach (var keyValue in keyValues)
            {
                var kv = keyValue.Split(EqualsChar);
                if (kv.Length != 2)
                    continue;
                var key = kv[0];
                var value = WebUtility.UrlDecode(kv[1]);
                ret[key] = value;
            }
            return ret;
        }

        public static void Wait2(this Task t)
        {
            try
            {
                t.Wait();
            }
            catch (AggregateException e)
            {
                var es = e.InnerExceptions.ToArray();
                if (es.Length > 0)
                    throw es[0];
            }
        }

        public static long FirstBlockHeight(NetworkType network)
        {
            switch (network)
            {
                case NetworkType.Ethereum:
                case NetworkType.Ropsten:
                    return 0;
                case NetworkType.Rsk:
                    return 1;
                default:
                    throw new ArgumentOutOfRangeException(nameof(network), network, null);
            }
        }
    }
}
