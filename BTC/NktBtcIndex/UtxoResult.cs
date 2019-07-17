using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace NktBtcIndex
{
    class UtxoResult
    {
        [JsonProperty("min_sigs")]
        public Int32 MinSigs;
        [JsonProperty("output_index")]
        public Int32 OutputIndex;
        [JsonProperty("txid")]
        public string TxId;
        [JsonProperty("value")]
        public string ValueString
        {
            set { Value = Convert.ToUInt64(value); }
            get { return Value.ToString(); }
        }
        [JsonIgnore]
        public ulong Value = 0;
    }
}
