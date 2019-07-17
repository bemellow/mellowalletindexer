using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public abstract class BinaryStore : IDisposable
    {
        private FileStream _file;
        private long _currentOffset;

        public BinaryStore(string path)
        {
            _file = new FileStream(path, FileMode.Append, FileAccess.Write, FileShare.Read);
            _currentOffset = _file.Length;
            _file.Seek(0, SeekOrigin.End);
        }

        public void Dispose()
        {
            _file?.Dispose();
            _file = null;
        }

        public struct InsertResult
        {
            public long Offset;
            public long Size;
        }

        protected abstract byte[] StringToBuffer(string input);

        public InsertResult Insert(string input)
        {
            var buffer = StringToBuffer(input);
            if (buffer.LongLength > Int32.MaxValue)
                throw new OverflowException();
            var ret = new InsertResult
            {
                Offset = _currentOffset,
                Size = buffer.Length
            };
            if (buffer.Length == 0)
                return ret;

            _currentOffset += buffer.LongLength;

            _file.Write(buffer, 0, buffer.Length);
            return ret;
        }

        public long Offset => _currentOffset;
    }
}
