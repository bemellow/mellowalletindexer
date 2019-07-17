using System.Linq;
using Newtonsoft.Json.Linq;
using Org.BouncyCastle.Math;

namespace EthereumClasses.InternalCommunication
{
    public class Block
    {
        public Block()
        {
        }

        public Block(JObject json)
        {
            Difficulty = json["difficulty"].Value<string>().HexStringToBigInteger();
            ExtraData = json["extraData"].Value<string>();
            GasLimit = json["gasLimit"].Value<string>().HexStringToBigInteger();
            GasUsed = json["gasUsed"].Value<string>().HexStringToBigInteger();
            BlockHash = json["hash"].Value<string>();
            LogsBloom = json["logsBloom"].Value<string>();
            Miner = json["miner"].Value<string>();
            Nonce = json["nonce"].Value<string>();
            Number = json["number"].Value<string>().HexStringToBigInteger();
            ParentHash = json["parentHash"].Value<string>();
            ReceiptsRoot = json["receiptsRoot"].Value<string>();
            Sha3Uncles = json["sha3Uncles"].Value<string>();
            Size = json["size"].Value<string>().HexStringToBigInteger();
            StateRoot = json["stateRoot"].Value<string>();
            Timestamp = json["timestamp"].Value<string>().HexStringToBigInteger();
            TotalDifficulty = json["totalDifficulty"].Value<string>().HexStringToBigInteger();
            TransactionsRoot = json["transactionsRoot"].Value<string>();
            Uncles = ((JArray)json["uncles"]).Select(x => Extensions.Value<string>(x)).ToArray();
        }

        public string BlockHash;
        public string ExtraData;
        public string LogsBloom;
        public string Miner;
        public string Nonce;
        public string ParentHash;
        public string ReceiptsRoot;
        public string Sha3Uncles;
        public string StateRoot;
        public string TransactionsRoot;
        public string[] Uncles;
        public BigInteger Difficulty;
        public BigInteger GasLimit;
        public BigInteger GasUsed;
        public BigInteger Number;
        public BigInteger Size;
        public BigInteger Timestamp;
        public BigInteger TotalDifficulty;
    }
}
