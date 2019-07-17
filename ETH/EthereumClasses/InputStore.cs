using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public class InputStore : BinaryStore
    {
        public InputStore(Configuration config) : base(config.InputsPath)
        {
        }

        protected override byte[] StringToBuffer(string input)
        {
            return Utility.ParseHexString(input).ToArray();
        }
    }
}
