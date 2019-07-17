using Newtonsoft.Json.Linq;
using Org.BouncyCastle.Math;

namespace EthereumClasses.InternalCommunication
{
    public class Transaction
    {
        public Transaction()
        {
        }

        public Transaction(JObject json)
        {
            BlockHash = json["blockHash"].Value<string>();
            BlockNumber = json["blockNumber"].Value<string>().HexStringToBigInteger();
            From = json["from"].Value<string>();
            Gas = json["gas"].Value<string>().HexStringToBigInteger();
            GasPrice = json["gasPrice"].Value<string>().HexStringToBigInteger();
            TransactionHash = json["hash"].Value<string>();
            Input = json["input"].Value<string>();
            Nonce = json["nonce"].Value<string>().HexStringToBigInteger();
            To = json["to"].Value<string>();
            TransactionIndex = json["transactionIndex"].Value<string>().HexStringToBigInteger();
            Value = json["value"].Value<string>().HexStringToBigInteger();
        }

        public string BlockHash;
        public string From;
        public string Input;
        public string To;
        public string TransactionHash;
        public BigInteger BlockNumber;
        public BigInteger Gas;
        public BigInteger GasPrice;
        public BigInteger Nonce;
        public BigInteger TransactionIndex;
        public BigInteger Value;
    }
}
