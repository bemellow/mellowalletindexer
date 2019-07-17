using Newtonsoft.Json;

namespace EthereumClasses
{
    public class TransactionRecord
    {
        [JsonProperty("txId")]
        public string Hash;
        [JsonProperty("input")]
        public string Sender;
        [JsonProperty("output")]
        public string Receiver;
        [JsonProperty("value")]
        public string Value;
        [JsonIgnore]
        public TimestampIndex.TransactionOrder Order
        {
            get { return _order; }
            set
            {
                Timestamp = value.block_timestamp;
                _order = value;
            }
        }
        [JsonIgnore]
        private TimestampIndex.TransactionOrder _order;
        [JsonProperty("timestamp")]
        public long Timestamp;
        [JsonProperty("blockHash")]
        public string BlockHash;
        [JsonProperty("blockNumber")]
        public long BlockHeight;
    }
}
