using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SQLite;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nethereum.Hex.HexTypes;
using Nethereum.RPC.Eth.DTOs;
using Nethereum.Web3;
using Newtonsoft.Json;

namespace EthereumClasses
{
    public class RequestedBlock
    {
        private BlockWithTransactions _block;
        private TransactionReceipt[] _receipts;

        public RequestedBlock(Web3 web3, long blockNo)
        {
            var requestBlockTask =
                            web3.Eth.Blocks.GetBlockWithTransactionsByNumber.SendRequestAsync(
                                new HexBigInteger(blockNo));
            Initialize(web3, requestBlockTask);
        }

        public RequestedBlock(Web3 web3, string hash)
        {
            var requestBlockTask = web3.Eth.Blocks.GetBlockWithTransactionsByHash.SendRequestAsync(hash);
            Initialize(web3, requestBlockTask);
        }

        private void Initialize(Web3 web3, Task<BlockWithTransactions> requestBlockTask)
        {
            requestBlockTask.Wait();
            _block = requestBlockTask.Result;
            if (Configuration.Get().RecordBinaryBuffers)
            {
                var receiptTasks =
                    _block.Transactions.Select(
                        x => web3.Eth.Transactions.GetTransactionReceipt.SendRequestAsync(x.TransactionHash))
                        .ToArray();
                Task.WaitAll(receiptTasks);
                _receipts = receiptTasks.Select(x => x.Result).ToArray();
            }
        }

        public BlockWithTransactions Block =>_block;
        public int TransactionCount => _block.Transactions.Length;
        public DateTime Timestamp => UnixTimeStampToDateTime(_block.Timestamp.ToLong());
        public TransactionReceipt[] Receipts => _receipts;
        
        private static DateTime UnixTimeStampToDateTime(long unixTimeStamp)
        {
            var ret = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc);
            ret = ret.AddSeconds(unixTimeStamp).ToLocalTime();
            return ret;
        }
        
    }
}
