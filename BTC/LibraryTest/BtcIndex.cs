using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace LibraryTest
{
    class BtcIndex : IDisposable
    {
        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr initialize_parser([MarshalAs(UnmanagedType.LPArray)] byte[] db_path);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void destroy_parser(IntPtr parser);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr parser_get_utxo(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr parser_get_balance(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr parser_get_sorted_tx_count(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        private IntPtr _parser = IntPtr.Zero;

        public BtcIndex(string dbPath)
        {
            _parser = initialize_parser(Utility.StringToUtf8(dbPath));
            if (_parser == IntPtr.Zero)
                throw new Exception("Failed to initialize parser");
        }

        public void Dispose()
        {
            if (_parser != IntPtr.Zero)
            {
                destroy_parser(_parser);
                _parser = IntPtr.Zero;
            }
        }

        public string GetUtxo(IEnumerable<string> addresses)
        {
            return GetUtxo(Newtonsoft.Json.JsonConvert.SerializeObject(addresses));
        }

        public string GetUtxo(string jsonAddresses)
        {
            return Utility.Utf8ToString(parser_get_utxo(_parser, Utility.StringToUtf8(jsonAddresses)));
        }

        public string GetBalance(IEnumerable<string> addresses)
        {
            return GetBalance(Newtonsoft.Json.JsonConvert.SerializeObject(addresses));
        }

        public string GetBalance(string jsonAddresses)
        {
            return Utility.Utf8ToString(parser_get_balance(_parser, Utility.StringToUtf8(jsonAddresses)));
        }

        public string GetTxs(IEnumerable<string> addresses)
        {
            return GetTxs(Newtonsoft.Json.JsonConvert.SerializeObject(addresses));
        }

        public string GetTxs(string jsonAddresses)
        {
            return Utility.Utf8ToString(parser_get_sorted_tx_count(_parser, Utility.StringToUtf8(jsonAddresses)));
        }

    }
}
