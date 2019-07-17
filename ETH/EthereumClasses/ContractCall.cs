using System.Collections.Generic;

namespace EthereumClasses
{
    public class ContractCall
    {
        public long TxId;
        public long TokenId;
        public int Function;
        public string Sender;
        public string Receiver;
        public string Value;
        public List<string> Parameters;
    }
}
