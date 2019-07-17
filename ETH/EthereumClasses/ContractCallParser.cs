using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nethereum.Util;

namespace EthereumClasses
{
    public abstract class ContractCallParser
    {
        protected Dictionary<string, int> _map = new Dictionary<string, int>();

        public ContractCallParser(IEnumerable<Tuple<string, int>> abis)
        {
            var hash = new Sha3Keccack();
            foreach (var tuple in abis)
            {
                var key = BitConverter.ToString(hash.CalculateHash(Encoding.UTF8.GetBytes(tuple.Item1)).Take(4).ToArray()).Replace("-", string.Empty).ToLower();
                _map[key] = tuple.Item2;
            }
        }

        public abstract ContractCall ParseInput(byte[] input, string sender, string receiver);
    }
}
