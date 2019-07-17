using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace NktBtcIndex
{
    static class Utility
    {
        public static byte[] StringToUtf8(string s)
        {
            var tempList = Encoding.UTF8.GetBytes(s).ToList();
            tempList.Add(0);
            return tempList.ToArray();
        }

        public static string Utf8ToString(IntPtr ptr)
        {
            int size = 0;
            while (Marshal.ReadByte(ptr, size) != 0)
                size++;
            var buffer = new byte[size];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            return Encoding.UTF8.GetString(buffer);
        }

        public static byte HexToVal(char c)
        {
            if (c >= '0' && c <= '9')
                return (byte)(c - '0');
            if (c >= 'a' && c <= 'f')
                return (byte)(c - 'a' + 10);
            if (c >= 'A' && c <= 'F')
                return (byte)(c - 'A' + 10);
            throw new Exception("Invalid hex");
        }

        public static byte[] HexStringToByteArray(string s)
        {
            var ret = new byte[s.Length / 2];
            int length = 0;
            byte accum = 0;
            int i = 0;
            foreach (var c in s)
            {
                accum <<= 4;
                accum |= HexToVal(c);
                if (++i == 2)
                {
                    ret[length++] = accum;
                    i = 0;
                }
            }
            return ret.ToArray();
        }

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
    }
}
