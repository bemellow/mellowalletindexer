using System.Linq;
using Newtonsoft.Json.Linq;

namespace EthereumClasses.InternalCommunication
{
    public class BlockWithTransactions : Block
    {
        public BlockWithTransactions()
        {
        }

        public BlockWithTransactions(JObject json) : base(json)
        {
            Transactions = ((JArray)json["transactions"]).Select(x => new Transaction((JObject)x)).ToArray();
        }

        public Transaction[] Transactions;
    }
}
