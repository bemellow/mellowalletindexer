using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace EthereumIndex
{
    class HistoryRequestParams
    {
        [JsonProperty("addresses")]
        public List<string> Addresses;
        [JsonProperty("max_txs")]
        public long MaxTxs;
        [JsonProperty("sort")]
        public int Ascending;
    }
}
