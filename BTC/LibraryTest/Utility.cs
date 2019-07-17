using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace LibraryTest
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
    }
}
