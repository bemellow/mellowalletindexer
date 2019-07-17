using Newtonsoft.Json.Linq;
using Org.BouncyCastle.Math;

namespace EthereumClasses.InternalCommunication
{
    public class TransactionReceipt
    {
        public TransactionReceipt()
        {
        }

        public TransactionReceipt(JObject json)
        {
            BlockHash = json["blockHash"].Value<string>();
            BlockNumber = json["blockNumber"].Value<string>().HexStringToBigInteger();
            ContractAddress = json["contractAddress"].Value<string>();
            CumulativeGasUsed = json["cumulativeGasUsed"].Value<string>().HexStringToBigInteger();
            GasUsed = json["gasUsed"].Value<string>().HexStringToBigInteger();
            TransactionHash = json["transactionHash"].Value<string>();
            TransactionIndex = json["transactionIndex"].Value<string>().HexStringToBigInteger();
            Logs = (JArray)json["logs"];
        }

        public string BlockHash;
        public BigInteger BlockNumber;
        public string ContractAddress;
        public BigInteger CumulativeGasUsed;
        public BigInteger GasUsed;
        public JArray Logs;
        public string TransactionHash;
        public BigInteger TransactionIndex;
    }
}
