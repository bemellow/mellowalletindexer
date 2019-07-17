using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Nethereum.Web3;
using EthereumClasses;
using Nethereum.RPC.Eth.DTOs;

namespace EthereumIndex
{
    class EthIndex : IDisposable
    {
        private static readonly log4net.ILog Log = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);
        private Web3 _web3;
        private Database _db;
        private Thread _updateThread;
        private bool _continueRunningBool = true;

        private static HashSet<Erc20Function> _erc20Functions = new HashSet<Erc20Function>
        {
            Erc20Function.Transfer,
            Erc20Function.TransferFrom,
        };

        private Erc20CallParser _parser = new Erc20CallParser(_erc20Functions);

        private Func<bool> _continueRunning;
        public Func<bool> ContinueRunning
        {
            set
            {
                lock (this)
                {
                    _continueRunning = value;
                }
            }
        }

        public IEnumerable<Token> Tokens => _db.Tokens.Values;

        public EthIndex(bool update = true)
        {
            var config = Configuration.Get();
            _web3 = new Web3(config.Url);
            _db = new Database(config);

            if (update)
            {
                Log.Info("Starting update thread.");
                _updateThread = new Thread(UpdateThreadFunction);
                _updateThread.Start();
            }
        }

        public void Dispose()
        {
            if (_updateThread != null)
            {
                _continueRunningBool = false;
                _updateThread.Join();
                _updateThread = null;
            }
            if (_db != null)
            {
                _db.Dispose();
                _db = null;
            }
        }

        private long GetLatestPublicBlock()
        {
            var t = _web3.Eth.Blocks.GetBlockNumber.SendRequestAsync();
            t.Wait();
            return t.Result.ToLong();
        }

        private bool InternalContinueRunning()
        {
            lock (this)
            {
                return _continueRunningBool && (_continueRunning == null || _continueRunning());
            }
        }

        static Task CommitDataAsync(List<RequestedBlock> blocks, Database db)
        {
            Log.Info($"Inserting {blocks.Count} blocks.");
            var ret = new Task(() =>
            {
                var elapsed = db.InsertBlocks(blocks);
                Log.Info($"Insertion completed in {elapsed} ms.");
            });
            ret.Start();
            return ret;
        }

        static void CommitData(List<RequestedBlock> blocks, Database db)
        {
            CommitDataAsync(blocks, db).Wait2();
        }

        private void DoInitialUpdate()
        {
            Log.Info("Doing initial update.");

            try
            {
                var sw = new Stopwatch();
                var queuedData = new List<RequestedBlock>(5000);
                Task commitTask = null;

                long queuedTxs = 0;
                while (InternalContinueRunning())
                {
                    var latestIndexedBlock = _db.GetIndexedBlockchainHeight();
                    var lastestPublicBlock = GetLatestPublicBlock() - 100;
                    if (lastestPublicBlock <= latestIndexedBlock)
                        break;
                    for (long blockNo = latestIndexedBlock + 1;
                        InternalContinueRunning() && blockNo <= lastestPublicBlock;
                        blockNo++)
                    {

                        RequestedBlock block = null;
                        for (int i = 0; ; i++)
                        {
                            try
                            {
                                block = new RequestedBlock(_web3, blockNo);
                            }
                            catch
                            {
                                if (i == 5)
                                    throw;
                                Thread.Sleep(1000 * (1 << i));
                            }
                            break;
                        }

                        Debug.Assert(block != null);

                        if (_db.BlockExistsInBlockchain(block.Block))
                            Debugger.Break();

                        queuedData.Add(block);
                        queuedTxs += block.TransactionCount;
                        var blockTs = block.Timestamp;
                        if (!sw.IsRunning || sw.ElapsedMilliseconds >= 1000)
                        {
                            Log.Info(
                                $"{blockNo}, {blockTs.ToString("yyyy-MM-dd HH:mm:ss zzzz")} ({(double) blockNo/lastestPublicBlock:P})");
                            //Log.Info($"{blockNo}, {(double)blockNo / lastestPublicBlock:P}, with {block.TransactionCount} txs and {queuedTxs} txs in queue...");
                            sw.Restart();
                        }

                        if (queuedTxs >= 50000)
                            commitTask = CommitData(commitTask, ref queuedData, ref queuedTxs, false);
                    }
                    commitTask = CommitData(commitTask, ref queuedData, ref queuedTxs, true);
                }
                commitTask = CommitData(commitTask, ref queuedData, ref queuedTxs, true);
            }
            catch (Database.RestartException)
            {
                Log.Info("Non-fatal exception caught.");
            }
        }

        private IEnumerable<List<ContractCall>> ProcessToken(Token token, NetworkType network)
        {
            var max = _db.GetLatestBlockForToken(token.Id);
            if (max == null)
                max = _db.GetBlockByHeight(Utility.FirstBlockHeight(network)).DbId;
            else
                max++;
            var blockIds = _db.GetBlocksWithTransactions(token.AddressId, max.Value);
            foreach (var i in blockIds)
            {
                Log.Info($"{token.Name}:{i}");
                var block = _db.GetBlockByHeight(i);
                var txTuples = _db.GetTransactionsToAddressInBlock(block.DbId, token.AddressId);
                var list = new List<ContractCall>();
                foreach (var txTuple in txTuples)
                {
                    for (int retries = 0;; retries++)
                    {
                        try
                        {
                            var task1 = _web3.Eth.Transactions.GetTransactionByHash.SendRequestAsync(txTuple.Item2);
                            var task2 = _web3.Eth.Transactions.GetTransactionReceipt.SendRequestAsync(txTuple.Item2);
                            task1.Wait();
                            task2.Wait();
                            var call = ProcessTokenTransaction(task1.Result, task2.Result, network, txTuple.Item1,
                                token.Id);
                            if (call != null)
                                list.Add(call);
                            break;
                        }
                        catch (NullReceiptException)
                        {
                            if (retries >= 5)
                                throw;
                            int seconds = 1 << retries;
                            Log.Warn($"Null receipt. Waiting for {seconds} seconds and retrying.");
                            Thread.Sleep(1000 * seconds);
                        }
                    }
                }
                yield return list;
            }
        }

        private ContractCall ProcessTokenTransaction(Transaction transaction, TransactionReceipt receipt, NetworkType network, long txId, long tokenId)
        {
            if (!EthereumClasses.Utility.TransactionSucceeded(transaction, receipt, network))
                return null;

            var array = EthereumClasses.Utility.ParseHexString(transaction.Input).ToArray();

            var call = _parser.ParseInput(
                array,
                transaction.From,
                transaction.To
            );

            if (call == null)
                return null;

            if (call.Receiver == null)
                Log.Error($"Receiver is null for TX {transaction.TransactionHash}, Input: {array}, {transaction.Input}");

            call.TxId = txId;
            call.TokenId = tokenId;

            return call;
        }

        private void PostprocessTransaction(long txid, Transaction transaction, Dictionary<string, long> tokenTxCounts)
        {
            Token token;
            if (string.IsNullOrEmpty(transaction.To))
                return;
            if (!_db.Tokens.TryGetValue(transaction.To, out token))
                return;
            var task = _web3.Eth.Transactions.GetTransactionReceipt.SendRequestAsync(transaction.TransactionHash);
            task.Wait();
            var call = ProcessTokenTransaction(transaction, task.Result, Configuration.Get().Network, txid, token.Id);
            if (call == null)
                return;

            long count = 0;
            tokenTxCounts.TryGetValue(token.Name, out count);
            tokenTxCounts[token.Name] = count + 1;

            _db.AddTokenTransaction(
                call.TxId,
                call.TokenId,
                call.Function,
                call.Sender,
                call.Receiver,
                call.Value
            );
        }

        private void AddAccum(List<ContractCall> accum, int minElements = 1)
        {
            if (accum.Count < minElements)
                return;
            using (var transaction = _db.BeginTransaction())
            {
                foreach (var call in accum)
                {
                    _db.AddTokenTransaction(
                        call.TxId,
                        call.TokenId,
                        call.Function,
                        call.Sender,
                        call.Receiver,
                        call.Value
                    );
                }
                transaction.Commit();
            }
            accum.Clear();
        }

        private void DoInitialTokenUpdate()
        {
            var config = Configuration.Get();
            var accum = new List<ContractCall>();
            foreach (var token in _db.Tokens.Values)
            {
                Log.Info($"Updating token {token.Name}.");
                foreach (var contractCalls in ProcessToken(token, config.Network))
                {
                    accum.AddRange(contractCalls);
                    AddAccum(accum, 4096);
                }
            }
            AddAccum(accum);
        }

        private Task CommitData(Task commitTask, ref List<RequestedBlock> queuedData, ref long queuedTxs, bool sync)
        {
            if (commitTask != null)
            {
                Log.Info("Syncing...");
                if (!commitTask.IsCompleted)
                    commitTask.Wait2();
                if (commitTask.IsFaulted)
                {
                    if (commitTask.Exception == null)
                        throw new Exception("Task IsFaulted but we don't know why.");
                    if (commitTask.Exception.InnerException != null)
                        throw commitTask.Exception.InnerException;
                    throw commitTask.Exception;
                }
                commitTask = null;
            }
            if (!sync)
                commitTask = CommitDataAsync(queuedData, _db);
            else
                CommitData(queuedData, _db);
            queuedData = new List<RequestedBlock>(5000);
            queuedTxs = 0;
            return commitTask;
        }

        private long UpdateToBlock(string hash)
        {
            lock (this)
            {
                return UpdateToBlock(new RequestedBlock(_web3, hash));
            }
        }

        private long UpdateToBlock(long blockNo, Stopwatch sw)
        {
            var block = new RequestedBlock(_web3, blockNo);
            sw.Restart();
            long ret;
            lock (this)
                ret = UpdateToBlock(block);
            sw.Stop();
            var ms = sw.ElapsedMilliseconds;
            var speed = (double) block.Block.Transactions.LongLength/(double) ms*1000;
            Log.Info($"Update completed in {ms} ms, speed: {speed}");
            return ret;
        }

        public class PushNewBlockResult
        {
            public string BlockRequired;
            public long BlocksReverted;
            public long DbId;
            public long NewHeight;
        }

        private long UpdateToBlock(RequestedBlock block)
        {
            var result = PushNewBlock(block);
            if (result.BlockRequired != null)
            {
                UpdateToBlock(result.BlockRequired);
                result = PushNewBlock(block);
            }
            if (result.BlocksReverted != 0)
                Log.Info($"Blocks reverted: {result.BlocksReverted}");
            return result.NewHeight;
        }

        private PushNewBlockResult PushNewBlock(RequestedBlock block)
        {
            var cr = _db.TryAddNewBlock(block.Block.ParentHash);
            if (cr.BlockRequired != null)
            {
                return new PushNewBlockResult
                {
                    BlockRequired = cr.BlockRequired,
                    BlocksReverted = 0,
                    DbId = -1,
                    NewHeight = -1,
                };
            }
            var tokenTxCounts = new Dictionary<string, long>();
            for (int retries = 0; ; retries++)
            {
                try
                {
                    using (var dbTx = _db.BeginTransaction())
                    {
                        if (cr.BlocksToRevert.Count > 0)
                        {
                            Log.Info($"Reverting {cr.BlocksToRevert.Count} blocks.");
                            _db.DeleteBlocks(cr.BlocksToRevert);
                        }
                        var blockId = _db.InsertBlock(block, (x, y) => PostprocessTransaction(x, y, tokenTxCounts));
                        var height = _db.AddNewBlock(block.Block.BlockHash, block.Block.ParentHash, blockId);
                        dbTx.Commit();

                        if (!Configuration.Get().Network.NetworkIsEthereum())
                            height++;

                        foreach (var kv in tokenTxCounts.Where(x => x.Value > 0))
                            Log.Info($"For token {kv.Key} processed {kv.Value} transactions.");

                        return new PushNewBlockResult
                        {
                            BlocksReverted = cr.BlocksToRevert.Count,
                            DbId = blockId,
                            NewHeight = height,
                        };
                    }
                }
                catch (NullReceiptException)
                {
                    if (retries >= 5)
                        throw;
                    int seconds = 1 << retries;
                    Log.Warn($"Null receipt. Waiting for {seconds} seconds and retrying.");
                    Thread.Sleep(1000 * seconds);
                }
            }
        }

        private void UpdateThreadFunction()
        {
            Log.Info("Update thread started.");
            Thread.CurrentThread.Name = "EthereumIndex update thread";
            DoInitialUpdate();
            if (!InternalContinueRunning())
                return;
            DoInitialTokenUpdate();
            if (!InternalContinueRunning())
                return;

            Log.Info("Initial update complete.");

            var latestIndexedBlock = _db.GetIndexedBlockchainHeight();
            var sw = new Stopwatch();
            while (InternalContinueRunning())
            {
                Thread.Sleep(1000);
                var latestPublicBlock = GetLatestPublicBlock();
                while (latestIndexedBlock < latestPublicBlock && InternalContinueRunning())
                {
                    Log.Info($"Updating to block height {latestIndexedBlock + 1}");
                    latestIndexedBlock = UpdateToBlock(latestIndexedBlock + 1, sw);
                }
            }
        }

        public List<TransactionRecord> GetHistory(IEnumerable<string> addresses, long maxTxs, bool oldestFirst)
        {
            lock (this)
            {
                return _db.GetHistory(addresses, maxTxs, oldestFirst);
            }
        }

        public List<TransactionRecord> GetHistory(Token token, IEnumerable<string> addresses, long maxTxs, bool oldestFirst)
        {
            lock (this)
            {
                return _db.GetHistory(token, addresses, maxTxs, oldestFirst);
            }
        }

        public BigInteger GetGasPrice()
        {
            lock (this)
            {
                return _db.GetGasPrice();
            }
        }
    }
}
