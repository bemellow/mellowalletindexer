using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public class TimestampIndex : IDisposable
    {
        public struct TimestampItem
        {
            public long TxBegin;
            public long TxEnd;
            public long Timestamp;
        }

        public struct TransactionOrder
        {
            public long block_timestamp;
            public int tx_index;
            public static int Cmp(TransactionOrder x, TransactionOrder y)
            {
                var a = x.block_timestamp - y.block_timestamp;
                if (x.block_timestamp < y.block_timestamp)
                    return -1;
                if (x.block_timestamp > y.block_timestamp)
                    return 1;
                return x.tx_index - y.tx_index;
            }
            public bool IsNull => block_timestamp == 0 && tx_index == 0;
        }

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr create_timestamp_index([MarshalAs(UnmanagedType.LPArray)] TimestampItem[] array, IntPtr n);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void destroy_timestamp_index(IntPtr index);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern TransactionOrder timestamp_index_get_timestamp(IntPtr index, long txId);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void timestamp_index_reload_data(IntPtr index, [MarshalAs(UnmanagedType.LPArray)] TimestampItem[] array, IntPtr n);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void timestamp_index_add_block(IntPtr index, ref TimestampItem item);

        private IntPtr _index;

        public TimestampIndex(TimestampItem[] data)
        {
            _index = create_timestamp_index(data, (IntPtr) data.LongLength);
            if (_index == IntPtr.Zero)
                throw new Exception("Failed to create timestamp index.");
        }

        public void Dispose()
        {
            if (_index != IntPtr.Zero)
            {
                destroy_timestamp_index(_index);
                _index = IntPtr.Zero;
            }
        }

        public TransactionOrder GetTimestamp(long txid)
        {
            var ret = timestamp_index_get_timestamp(_index, txid);
            if (ret.IsNull)
                throw new Exception($"Could not find timestamp for txid {txid}.");
            return ret;
        }

        public void Reload(TimestampItem[] data)
        {
            timestamp_index_reload_data(_index, data, (IntPtr)data.LongLength);
        }

        public void AddBlock(TimestampItem item)
        {
            timestamp_index_add_block(_index, ref item);
        }
    }
}
