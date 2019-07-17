using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using BitcoinLib.Responses;
using BitcoinLib.RPC.Connector;
using BitcoinLib.RPC.Specifications;
using BitcoinLib.Services.Coins.Bitcoin;

namespace RpcTest
{
    public class RawBlockResponse
    {
        public RawBlockResponse()
        {
            Tx = new List<string>();
        }

        public List<string> Tx { get; set; }
        public string Hash { get; set; }
        public int Confirmations { get; set; }
        public int Size { get; set; }
        public int Height { get; set; }
        public int Version { get; set; }
        public string MerkleRoot { get; set; }
        public double Difficulty { get; set; }
        public string ChainWork { get; set; }
        public string PreviousBlockHash { get; set; }
        public string NextBlockHash { get; set; }
        public string Bits { get; set; }
        public int Time { get; set; }
        public string Nonce { get; set; }
    }

    class Program
    {
        static void Main(string[] args)
        {
            var btc = new BitcoinService("http://gethfull:8332", "coinfabrik", "0oOI078hIwho6nyWyeUJBz6YvKFmlm8t", "", 10);
            var conn = new RpcConnector(btc);
            long last = -1;
            while (true)
            {
                var latestBlock = conn.MakeRequest<long>(RpcMethods.getblockcount);
                if (latestBlock == last)
                {
                    Thread.Sleep(1000);
                    continue;
                }
                var latestBlockHash = conn.MakeRequest<string>(RpcMethods.getblockhash, new object[] {latestBlock});
                var now = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
                Console.WriteLine($"{now} - New block: {latestBlockHash}");
                last = latestBlock;
            }
            //var block = conn.MakeRequest<string>(RpcMethods.getblock, new object[]{ latestBlockHash, 0 });
            
            //Console.WriteLine(block);
        }
    }
}
