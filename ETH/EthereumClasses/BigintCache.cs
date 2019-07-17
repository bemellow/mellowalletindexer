using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SQLite;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public class InconsistentAddressEncodingException : Exception
    {
        public readonly long Expected;
        public readonly long Actual;
        public readonly long? LastGood;

        public InconsistentAddressEncodingException(long expected, long actual, long? lastGood)
        {
            Expected = expected;
            Actual = actual;
            LastGood = lastGood;
        }

        public override string Message => $"Internal error: a bigint was inconsistently encoded! Expected: {Expected}, actual: {Actual}";
    }

    public class BigintCache : IDisposable
    {
        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr create_cache(long size);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void release_cache(IntPtr cache);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern long encode_bigint(IntPtr cache, string value, bool ro, ref bool newId);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern bool decode_id(IntPtr cache, [MarshalAs(UnmanagedType.LPArray)] byte[] value, long id);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern long finish_preloading(IntPtr cache);

        private IntPtr _cache;
        private SQLiteConnection _conn;
        private SQLiteCommand _cmd;
        private bool _rebuilding = false;

        public BigintCache(SQLiteConnection conn)
        {
            _conn = conn;
            _cmd = new SQLiteCommand("insert into bigints (id, value) values (@id, @value);", _conn);
#if true
            long rows = 0;
            using (var comm = new SQLiteCommand("select max(id) from bigints;", conn))
            using (var reader = comm.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (!reader.Read())
                    throw new Exception();
                var value = reader[0];
                if (value is DBNull)
                    rows = 0;
                else
                    rows = (long) value + 1;
            }

            Console.WriteLine($"Inserting {rows} elements into the cache.");
            _cache = create_cache(rows);

            if (rows != 0)
            {
                _rebuilding = true;
                long testId = 1000000;
                string test = null;
                long? lastGood = null;
                try
                {
                    using (var comm = new SQLiteCommand("select value, id from bigints order by id asc;", conn))
                    using (var reader = comm.ExecuteReader(CommandBehavior.SequentialAccess))
                    {
                        while (reader.Read())
                        {
                            var value = (string) reader[0];
                            var id = (long) reader[1];
                            var encoded = Encode(value);
                            if (id == testId)
                                test = value;
                            if (encoded != id)
                                throw new InconsistentAddressEncodingException(id, encoded, lastGood);
                            lastGood = id;
                        }
                    }
                }
                catch
                {
                    Dispose();
                    throw;
                }
            }
#else
            _cache = create_cache(0);
#endif
            finish_preloading(_cache);
            _rebuilding = false;
        }

        public void Dispose()
        {
            if (_cache != IntPtr.Zero)
            {
                release_cache(_cache);
                _cache = IntPtr.Zero;
            }
            if (_cmd != null)
            {
                _cmd.Dispose();
                _cmd = null;
            }
        }

        public long? EncodeReadOnly(string s)
        {
            s = s ?? string.Empty;
            bool notFound = false;
            var ret = encode_bigint(_cache, s, true, ref notFound);
            if (notFound)
                return null;
            return ret;
        }

        public long Encode(string s)
        {
            s = s ?? string.Empty;
            bool newId = false;
            var ret = encode_bigint(_cache, s, false, ref newId);
            if (ret < 0)
                throw new Exception($"Failed to encode {s}");
            if (newId && !_rebuilding)
            {
                _cmd.Reset();
                _cmd.Parameters.Clear();
                _cmd.Parameters.Add(new SQLiteParameter("id", ret));
                _cmd.Parameters.Add(new SQLiteParameter("value", s));
                _cmd.ExecuteNonQuery();
            }
            return ret;
        }

        public string Decode(long id)
        {
            var temp = new byte[64];
            if (!decode_id(_cache, temp, id))
                return null;
            int i = 0;
            while (temp[i] != 0)
                i++;
            return Encoding.UTF8.GetString(temp, 0, i);
        }
    }
}
