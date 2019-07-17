using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public class LogsStore : BinaryStore
    {
        public LogsStore(Configuration config) : base(config.LogsPath)
        {
        }

        protected override byte[] StringToBuffer(string input)
        {
            return Encoding.UTF8.GetBytes(input);
        }
    }
}
