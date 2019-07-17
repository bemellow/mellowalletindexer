using System;
using System.Collections.Generic;
using System.Data;
using System.Data.Common;
using System.Data.SQLite;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Nethereum.RPC.Eth.DTOs;
using Newtonsoft.Json;

namespace EthereumClasses
{
    public class Database : IDisposable
    {
        private static SQLiteConnection OpenDb(Configuration config)
        {
            var ret = new SQLiteConnection($"Data source={config.DbPath};Version=3");
            ret.Open();
            return ret;
        }

        private static void CreateTokenTables(SQLiteConnection conn)
        {
            using (var cmd = new SQLiteCommand(
$@"create table if not exists tokens (
    id integer primary key,
    name text collate nocase unique,
    address text collate nocase
);

create table if not exists token_txs (
    id integer primary key,
    txs_id integer,
    tokens_id integer,
    function_id integer,
    sender integer,
    receiver integer,
    amount_wei text,
    extra1 text,
    extra2 text
);

create index if not exists token_txs_by_txs_id on token_txs (txs_id);
create index if not exists token_txs_by_tokens_id on token_txs (tokens_id);
create index if not exists token_txs_by_sender on token_txs (sender);
create index if not exists token_txs_by_receiver on token_txs (receiver);
", conn))
            {
                cmd.ExecuteNonQuery();
            }
        }
        
        private static void InitializeDb(Configuration config)
        {
            if (File.Exists(config.DbPath))
            {
                using (var conn = OpenDb(config))
                    CreateTokenTables(conn);
                return;
            }
            using (var conn = OpenDb(config))
            {
                CreateTokenTables(conn);
                using (var comm = new SQLiteCommand(@"
create table bigints(id integer primary key, value text collate nocase);
create table txs(
    id integer primary key,
    hash text collate nocase,
    sender integer,
    receiver integer,
    amount_wei text,
    blocks_id integer,
    gas_price text,
    gas_limit text,
    contract_address text,
    gas_used text,
    input_offset integer,
    input_size integer,
    logs_offset integer,
    logs_size integer
);
create table blocks(
    id integer primary key,
    hash text collate nocase,
    previous_hash text collate nocase,
    miner integer,
    previous_blocks_id integer,
    timestamp integer,
    first_transaction_id integer,
    transaction_count integer,
    average_gasprice text
);
create table blockchain_head (hash text collate nocase);"

#if true
+ @"
create index bigints_by_value on bigints(value);
create index txs_by_sender on txs (sender);
create index txs_by_receiver on txs (receiver);
create index txs_by_block on txs (blocks_id);
create index blocks_by_hash on blocks (hash);
"
#endif
                    ,
                    conn
                    ))
                    comm.ExecuteNonQuery();
            }
        }

        private SQLiteConnection _conn;
        private BigintCache _cache;
        private SQLiteCommand _insertBlock;
        private SQLiteCommand _insertTransaction;
        private SQLiteCommand _readTx;
        private SQLiteCommand _readGasPrices;
        private SQLiteCommand _readAverageGasPrice;
        private SQLiteCommand _writeAverageGasPrice;
        private SQLiteCommand _getHistoryByTxId;
        private SQLiteCommand _getTxsByAddress;
        private SQLiteCommand _getTokenTxsByAddress;
        private SQLiteCommand _insertTokenTransaction;
        private SQLiteCommand _getTransactionsToAddressInBlock;
        private InputStore _inputStore;
        private LogsStore _logsStore;
        private object _blockchainLock = new object();
        private AbstractBlockchain _blockchain;
        private TimestampIndex _timestampIndex;
        private long _nextTxId;

        public Database(Configuration config)
        {
            InitializeDb(config);
            _conn = OpenDb(config);
            while (true)
            {
                try
                {
                    _cache = new BigintCache(_conn);
                    break;
                }
                catch (InconsistentAddressEncodingException ex)
                {
                    if (!ex.LastGood.HasValue)
                        throw;
                    Console.WriteLine("Warning: Addresses have been inconsistently encoded. Rolling database back.");
                    if (!CleanUpDatabase(ex.LastGood.Value))
                        throw;
                }
            }
            InitBlockchain();
            InitializeTimestampIndex();

            #region SQLite commands
            _insertBlock = new SQLiteCommand(
@"insert into blocks (
    hash,
    previous_hash,
    miner,
    timestamp,
    first_transaction_id,
    transaction_count,
    average_gasprice
) values (
    @hash,
    @previous_hash,
    @miner,
    @timestamp,
    @first_transaction_id,
    @transaction_count,
    @average_gasprice
);"
                , _conn);
            _insertTransaction = new SQLiteCommand(
@"insert into txs (
    id,
    hash,
    sender,
    receiver,
    amount_wei,
    blocks_id,
    input_offset,
    input_size,
    gas_price,
    gas_limit,
    contract_address,
    gas_used,
    logs_offset,
    logs_size
) values (
    @id,
    @hash,
    @sender,
    @receiver,
    @amount_wei,
    @blocks_id,
    @input_offset,
    @input_size,
    @gas_price,
    @gas_limit,
    @contract_address,
    @gas_used,
    @logs_offset,
    @logs_size
);"
                , _conn);
            _readTx = new SQLiteCommand("select txs.hash, txs.sender, txs.receiver, txs.amount_wei, blocks.hash from txs inner join blocks on blocks.id = txs.blocks_id where txs.sender = @id or receiver = @id;", _conn);
            _readGasPrices = new SQLiteCommand("select gas_price from txs where blocks_id = @blocks_id;", _conn);
            _readAverageGasPrice = new SQLiteCommand("select average_gasprice from blocks where id = @id;", _conn);
            _writeAverageGasPrice = new SQLiteCommand("update blocks set average_gasprice = @average_gasprice where id = @id;");
            _getHistoryByTxId = new SQLiteCommand("select txs.hash, txs.sender, txs.receiver, txs.amount_wei, blocks.hash from txs inner join blocks on blocks.id = txs.blocks_id where txs.id = @id;", _conn);
            _getTxsByAddress = new SQLiteCommand("select id from txs where sender = @id or receiver = @id;", _conn);
            _getTokenTxsByAddress = new SQLiteCommand("select txs_id, amount_wei, sender, receiver from token_txs where tokens_id = @tokens_id and (sender = @id or receiver = @id);", _conn);
            _insertTokenTransaction = new SQLiteCommand("insert into token_txs (txs_id, tokens_id, function_id, sender, receiver, amount_wei) values (@txs_id, @tokens_id, @function_id, @sender, @receiver, @amount_wei);", _conn);
            _getTransactionsToAddressInBlock = new SQLiteCommand("select id, hash from txs where receiver = @receiver and blocks_id = @blocks_id;", _conn);
            #endregion

            _inputStore = new InputStore(config);
            _logsStore = new LogsStore(config);

            _nextTxId = GetNextTxId();
        }

        private long? GetMinBlockTx(long lastGoodAddressId)
        {
            var command = $"select min(first_transaction_id) from blocks where miner > {lastGoodAddressId};";
            using (var cmd = new SQLiteCommand(command, _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (reader.Read())
                {
                    var ret = (long) reader[0];
                    Console.WriteLine($"GetMinBlockTx(): {command} = {ret}");
                    return ret;
                }
            }
            return null;
        }

        private long? GetMinTxTx(long lastGoodAddressId)
        {
            var command1 = $"select min(id) from txs where sender = {lastGoodAddressId} or receiver = {lastGoodAddressId};";
            using (var cmd = new SQLiteCommand(command1, _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (reader.Read())
                {
                    var txId = (long) reader[0];
                    Console.WriteLine($"GetMinTxTx(): {command1} = {txId}");
                    var command2 = $"select min(id) from txs where id >= {txId} and (sender > {lastGoodAddressId} or receiver > {lastGoodAddressId});";
                    using (var cmd2 = new SQLiteCommand(command2, _conn))
                    using (var reader2 = cmd2.ExecuteReader(CommandBehavior.SequentialAccess))
                    {
                        if (reader2.Read())
                        {
                            var ret = (long) reader2[0];
                            Console.WriteLine($"GetMinTxTx(): {command2} = {ret}");
                            return ret;
                        }
                    }
                }
            }
            return null;
        }

        private long? GetMinTokenTx(long lastGoodAddressId)
        {
            var command = $"select min(txs_id) from token_txs where sender > {lastGoodAddressId} or receiver > {lastGoodAddressId};";
            using (var cmd = new SQLiteCommand(command, _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (reader.Read())
                {
                    var ret = (long) reader[0];
                    Console.WriteLine($"GetMinTokenTx(): {command} = {ret}");
                    return ret;
                }
            }
            return null;
        }

        private bool CleanUpDatabase(long lastGoodAddressId)
        {
            using (var dbTx = BeginTransaction())
            {
                var ids = new[]
                {
                    GetMinBlockTx(lastGoodAddressId),
                    GetMinTxTx(lastGoodAddressId),
                    GetMinTokenTx(lastGoodAddressId),
                }
                    .Where(x => x != null)
                    .Select(x => x.Value)
                    .OrderBy(x => x)
                    .ToArray();
                if (ids.Length == 0)
                    return false;

                var minTxId = ids[0];
                Console.WriteLine($"CleanUpDatabase(): minTxId = {minTxId}");
                long blockId;

                using (var cmd = new SQLiteCommand($"select id, first_transaction_id from blocks where first_transaction_id <= {minTxId} and {minTxId} < first_transaction_id + transaction_count;", _conn))
                using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                {
                    if (!reader.Read())
                        return false;
                    blockId = (long) reader[0];
                    minTxId = (long) reader[1];
                }

                Console.WriteLine($"CleanUpDatabase(): blockId = {blockId}");
                Console.WriteLine($"CleanUpDatabase(): minTxId = {minTxId}");
                Console.WriteLine($"CleanUpDatabase(): lastGoodAddressId = {lastGoodAddressId}");

                var list = new[]
                {
                    new Tuple<string, string>($"delete from blocks where id >= {blockId};", "Deleting blocks..."),
                    new Tuple<string, string>($"delete from txs where id >= {minTxId};", "Deleting transactions..."),
                    new Tuple<string, string>($"delete from token_txs where id >= {minTxId};", "Deleting token transactions..."),
                    new Tuple<string, string>($"delete from bigints where id > {lastGoodAddressId};", "Deleting addresses..."),
                };

                foreach (var tuple in list)
                    Console.WriteLine(tuple.Item1);

                foreach (var tuple in list)
                {
                    Console.WriteLine(tuple.Item2);
                    using (var cmd = new SQLiteCommand(tuple.Item1, _conn))
                        cmd.ExecuteNonQuery();
                }

                dbTx.Commit();
                return true;
            }
        }

        private IEnumerable<TimestampIndex.TimestampItem> GetTimestamps()
        {
            using (var cmd = new SQLiteCommand("select timestamp, first_transaction_id, transaction_count from blocks where transaction_count != 0;", _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                while (reader.Read())
                {
                    var timestamp = (long)reader[0];
                    var txBegin = (long)reader[1];
                    var count = (long)reader[2];
                    yield return new TimestampIndex.TimestampItem
                    {
                        Timestamp = timestamp,
                        TxBegin = txBegin,
                        TxEnd = txBegin + count,
                    };
                }
            }
        }

        private void InitBlockchain()
        {
            lock (_blockchainLock)
            {
                if (_blockchain != null)
                    return;
                _blockchain = new CppBlockchain(this);
            }
        }

        private void InitializeTimestampIndex()
        {
            _timestampIndex = new TimestampIndex(GetTimestamps().ToArray());
        }

        private void ReloadTimestampIndex()
        {
            _timestampIndex.Reload(GetTimestamps().ToArray());
        }

        public bool AnyTxs()
        {
            using (var cmd = new SQLiteCommand("select id from txs limit 1;", _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                return reader.Read();
        }

        public long GetNextTxId()
        {
            if (!AnyTxs())
                return 1;
            using (var cmd = new SQLiteCommand("select max(id) from txs limit 1;", _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (!reader.Read())
                    return 1;
                return (long) reader[0] + 1;
            }
        }

        public void Dispose()
        {
            if (_conn != null)
            {
                _conn.Dispose();
                _conn = null;
            }
            if (_blockchain != null)
            {
                _blockchain.Dispose();
                _blockchain = null;
            }
            if (_cache != null)
            {
                _cache.Dispose();
                _cache = null;
            }
            if (_insertBlock != null)
            {
                _insertBlock.Dispose();
                _insertBlock = null;
            }
            if (_insertTransaction != null)
            {
                _insertTransaction.Dispose();
                _insertTransaction = null;
            }
            if (_readTx != null)
            {
                _readTx.Dispose();
                _readTx = null;
            }
            if (_readGasPrices != null)
            {
                _readGasPrices.Dispose();
                _readGasPrices = null;
            }
            if (_readAverageGasPrice != null)
            {
                _readAverageGasPrice.Dispose();
                _readAverageGasPrice = null;
            }
            if (_writeAverageGasPrice != null)
            {
                _writeAverageGasPrice.Dispose();
                _writeAverageGasPrice = null;
            }
            if (_getHistoryByTxId != null)
            {
                _getHistoryByTxId.Dispose();
                _getHistoryByTxId = null;
            }
            if (_getTxsByAddress != null)
            {
                _getTxsByAddress.Dispose();
                _getTxsByAddress = null;
            }
            if (_getTokenTxsByAddress != null)
            {
                _getTokenTxsByAddress.Dispose();
                _getTokenTxsByAddress = null;
            }
            if (_insertTokenTransaction != null)
            {
                _insertTokenTransaction.Dispose();
                _insertTokenTransaction = null;
            }
            if (_getTransactionsToAddressInBlock != null)
            {
                _getTransactionsToAddressInBlock.Dispose();
                _getTransactionsToAddressInBlock = null;
            }
            if (_inputStore != null)
            {
                _inputStore.Dispose();
                _inputStore = null;
            }
            if (_logsStore != null)
            {
                _logsStore.Dispose();
                _logsStore = null;
            }
        }

        public long GetIndexedBlockchainHeight()
        {
            return Blockchain.Height;
        }

        public DbTransaction BeginTransaction()
        {
            return _conn.BeginTransaction();
        }

        public class RestartException : Exception {}

        public long InsertBlocks(IEnumerable<RequestedBlock> blocks)
        {
            var sw = new Stopwatch();
            sw.Start();
            using (var dbTx = BeginTransaction())
            {
                foreach (var block in blocks)
                {
                    if (_blockchain != null)
                    {
                        var br = _blockchain.TryAddNewBlock(block.Block.ParentHash);
                        DeleteBlocks(br.BlocksToRevert);
                        if (br.BlockRequired != null)
                        {
                            dbTx.Commit();
                            throw new RestartException();
                        }
                    }
                    var blockId = InsertBlock(block);
                    if (_blockchain != null)
                        _blockchain.AddNewBlock(block.Block.BlockHash, block.Block.ParentHash, blockId);
                }
                InitBlockchain();

                dbTx.Commit();
            }
            sw.Stop();
            return sw.ElapsedMilliseconds;
        }

        public bool BlockExistsInBlockchain(Block block)
        {
            lock (_blockchainLock)
                return Blockchain.GetBlockByHash(block.BlockHash) != null;
        }

        public long InsertBlock(RequestedBlock block, Action<long, Transaction> processTransaction = null)
        {
            if (_blockchain != null && BlockExistsInBlockchain(block.Block))
                throw new Exception($"Attempting to add duplicate block {block.Block.BlockHash}.");

            var blockId = InsertOnlyBlock(block.Block, _nextTxId);
            InsertTransactions(block.Block.Transactions, block.Receipts, blockId, _nextTxId, processTransaction);
            _nextTxId += block.Block.Transactions.Length;
            return blockId;
        }

        private long InsertOnlyBlock(BlockWithTransactions block, long firstTransactionId)
        {
            var timestamp = block.Timestamp.ToLong();

            var cmd = _insertBlock;
            cmd.Reset();
            cmd.Parameters.Clear();
            cmd.Parameters.Add(new SQLiteParameter("hash", block.BlockHash));
            cmd.Parameters.Add(new SQLiteParameter("previous_hash", block.ParentHash));
            cmd.Parameters.Add(new SQLiteParameter("miner", _cache.Encode(block.Miner)));
            cmd.Parameters.Add(new SQLiteParameter("timestamp", timestamp));
            cmd.Parameters.Add(new SQLiteParameter("first_transaction_id", firstTransactionId));
            cmd.Parameters.Add(new SQLiteParameter("transaction_count", block.Transactions.LongLength));
            cmd.Parameters.Add(new SQLiteParameter("average_gasprice", null));

            cmd.ExecuteNonQuery();

            _timestampIndex.AddBlock(new TimestampIndex.TimestampItem
            {
                Timestamp = timestamp,
                TxBegin = firstTransactionId,
                TxEnd = firstTransactionId + block.Transactions.LongLength,
            });

            return _conn.LastInsertRowId;
        }

        private void InsertTransactions(Transaction[] transactions, TransactionReceipt[] receipts, long blockId, long firstTxId, Action<long, Transaction> processTransaction)
        {
            if (receipts != null && transactions.Length != receipts.Length)
                throw new Exception("Assumption violated!");
            int i = 0;
            var config = Configuration.Get();
            var id = firstTxId;
            foreach (var tx in transactions)
            {
                var txId = id++;
                InsertTransaction(txId, tx, receipts?[i++], config, blockId);
                if (processTransaction != null)
                    processTransaction(txId, tx);
            }
        }

        private void InsertTransaction(long id, Transaction tx, TransactionReceipt receipt, Configuration config, long blockId)
        {
            var cmd = _insertTransaction;

            cmd.Reset();
            cmd.Parameters.Clear();
            cmd.Parameters.Add(new SQLiteParameter("id", id));
            cmd.Parameters.Add(new SQLiteParameter("hash", tx.TransactionHash));
            cmd.Parameters.Add(new SQLiteParameter("sender", _cache.Encode(tx.From ?? "0x00")));
            cmd.Parameters.Add(new SQLiteParameter("receiver", _cache.Encode(tx.To ?? "0x00")));
            cmd.Parameters.Add(new SQLiteParameter("amount_wei", tx.Value.Value.ToString()));
            cmd.Parameters.Add(new SQLiteParameter("blocks_id", blockId));

            if (config.RecordBinaryBuffers)
            {
                if (tx.Input != "0x")
                {
                    var input = _inputStore.Insert(tx.Input);
                    cmd.Parameters.Add(new SQLiteParameter("input_offset", input.Offset));
                    cmd.Parameters.Add(new SQLiteParameter("input_size", input.Size));
                }
                else
                {
                    cmd.Parameters.Add(new SQLiteParameter("input_offset", _inputStore.Offset));
                    cmd.Parameters.Add(new SQLiteParameter("input_size", 0));
                }
            }
            else
            {
                cmd.Parameters.Add(new SQLiteParameter("input_offset", null));
                cmd.Parameters.Add(new SQLiteParameter("input_size", null));
            }
            cmd.Parameters.Add(new SQLiteParameter("gas_price", tx.GasPrice.Value.ToString()));
            cmd.Parameters.Add(new SQLiteParameter("gas_limit", tx.Gas.Value.ToString()));
            if (receipt != null)
            {
                if (receipt.ContractAddress != null)
                    cmd.Parameters.Add(new SQLiteParameter("contract_address", _cache.Encode(receipt.ContractAddress)));
                else
                    cmd.Parameters.Add(new SQLiteParameter("contract_address", null));
                cmd.Parameters.Add(new SQLiteParameter("gas_used", receipt.GasUsed.Value.ToString()));
            }
            else
            {
                cmd.Parameters.Add(new SQLiteParameter("contract_address", null));
                cmd.Parameters.Add(new SQLiteParameter("gas_used", null));
            }

            if (config.RecordBinaryBuffers)
            {
                if (receipt.Logs.Count != 0)
                {
                    var logs = _logsStore.Insert(receipt.Logs.ToString(Formatting.None));
                    cmd.Parameters.Add(new SQLiteParameter("logs_offset", logs.Offset));
                    cmd.Parameters.Add(new SQLiteParameter("logs_size", logs.Size));
                }
                else
                {
                    cmd.Parameters.Add(new SQLiteParameter("logs_offset", _logsStore.Offset));
                    cmd.Parameters.Add(new SQLiteParameter("logs_size", 0));
                }
            }
            else
            {
                cmd.Parameters.Add(new SQLiteParameter("logs_offset", null));
                cmd.Parameters.Add(new SQLiteParameter("logs_size", null));
            }
            cmd.ExecuteNonQuery();
        }

        public class DatabaseBlock
        {
            public long Id;
            public string Hash;
            public string PreviousHash;
        }

        public IEnumerable<DatabaseBlock> GetAllBlocks()
        {
            using (var cmd = new SQLiteCommand("select id, hash, previous_hash from blocks;", _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                while (reader.Read())
                {
                    var block = new DatabaseBlock
                    {
                        Id = (long) reader[0],
                        Hash = (string) reader[1],
                        PreviousHash = (string) reader[2]
                    };
                    yield return block;
                }
            }
        }

        private string _blockchainHead;

        public string BlockchainHead
        {
            get
            {
                if (_blockchainHead == null)
                {
                    try
                    {
                        using (var cmd = new SQLiteCommand("select hash from blockchain_head limit 1;", _conn))
                        using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                        {
                            if (!reader.Read())
                                return null;
                            _blockchainHead = (string) reader[0];
                        }
                    }
                    catch
                    {
                        return null;
                    }
                }
                return _blockchainHead;
            }
            set
            {
                _blockchainHead = value.ToLower();
                using (
                    var cmd =
                        new SQLiteCommand(
                            "delete from blockchain_head; insert into blockchain_head (hash) values (@hash);", _conn))
                {
                    cmd.Reset();
                    cmd.Parameters.Add(new SQLiteParameter("hash", _blockchainHead));
                    cmd.ExecuteNonQuery();
                }
            }
        }

        private AbstractBlockchain Blockchain
        {
            get
            {
                InitBlockchain();
                return _blockchain;
            }
        }

        private Tuple<long, long> GetTxsForBlock(long blockId)
        {
            using (var cmd = new SQLiteCommand("select first_transaction_id, transaction_count from blocks where id = @id;", _conn))
            {
                cmd.Parameters.Add(new SQLiteParameter("id", blockId));
                using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                {
                    if (!reader.Read())
                        return null;
                    var first = (long)reader[0];
                    var count = (long) reader[1];
                    return new Tuple<long, long>(first, first + count);
                }
            }
        }

        private void DeleteTokenTxs(IEnumerable<Tuple<long, long>> ids)
        {
            using (var cmd = new SQLiteCommand("delete from token_txs where txs_id >= @begin and txs_id < @end;", _conn))
            {
                foreach (var tuple in ids)
                {
                    cmd.Reset();
                    cmd.Parameters.Add(new SQLiteParameter("begin", tuple.Item1));
                    cmd.Parameters.Add(new SQLiteParameter("end", tuple.Item2));
                    cmd.ExecuteNonQuery();
                }
            }
        }

        public void DeleteBlocks(List<AbstractBlockchain.ChainReorganizationBlock> blocksToRevert)
        {
            if (blocksToRevert.Count == 0)
                return;
            var ids = new List<Tuple<long, long>>();
            using (var deleteBlocks = new SQLiteCommand("delete from blocks where id = @id;", _conn))
            using (var deleteTxs = new SQLiteCommand("delete from txs where blocks_id = @id;", _conn))
            {
                foreach (var block in blocksToRevert)
                {
                    var tuple = GetTxsForBlock(block.DbId);
                    if (tuple != null)
                        ids.Add(tuple);
                    deleteBlocks.Reset();
                    deleteBlocks.Parameters.Add(new SQLiteParameter("id", block.DbId));
                    deleteBlocks.ExecuteNonQuery();
                    deleteTxs.Reset();
                    deleteTxs.Parameters.Add(new SQLiteParameter("id", block.DbId));
                    deleteTxs.ExecuteNonQuery();
                    Blockchain.RevertBlock(block.Hash);
                }
                _nextTxId = GetNextTxId();
                DeleteTokenTxs(ids);
                ReloadTimestampIndex();
            }
        }

        private IEnumerable<long> MapAddresses(IEnumerable<string> addresses)
        {
            return addresses
                .Select(MapAddress)
                .Where(x => x != null)
                .Select(x => x.Value);
        }

        private long? MapAddress(string address)
        {
            var ret = _cache.EncodeReadOnly(address);
            if (ret != null)
                Console.WriteLine($"Map address {address} = {ret}");
            return ret;
        }

        private class TransactionId
        {
            public long TxId;
            public TimestampIndex.TransactionOrder Order;

            protected virtual TransactionRecord InternalGetTransactionRecord(SQLiteDataReader reader, BigintCache cache,
                AbstractBlockchain blockchain)
            {
                if (!reader.Read())
                    return null;
                var hash = (string)reader[0];
                var sender = (long)reader[1];
                var receiver = (long)reader[2];
                var value = (string)reader[3];
                var blockHash = (string)reader[4];
                var block = blockchain.GetBlockByHash(blockHash);

                return new TransactionRecord
                {
                    Hash = hash,
                    Sender = cache?.Decode(sender),
                    Receiver = cache?.Decode(receiver),
                    Value = value,
                    BlockHash = blockHash,
                    BlockHeight = block.Height,
                    Order = Order,
                };
            }

            public virtual TransactionRecord GetTransactionRecord(SQLiteDataReader reader, BigintCache cache, AbstractBlockchain blockchain)
            {
                return InternalGetTransactionRecord(reader, cache, blockchain);
            }
        }

        private class TokenTransactionId : TransactionId
        {
            public string Amount;
            public string Sender;
            public string Receiver;

            public override TransactionRecord GetTransactionRecord(SQLiteDataReader reader, BigintCache cache, AbstractBlockchain blockchain)
            {
                var ret = InternalGetTransactionRecord(reader, null, blockchain);
                if (ret == null)
                    return ret;
                ret.Value = Amount;
                ret.Sender = Sender;
                ret.Receiver = Receiver;
                return ret;
            }
        }

        private IEnumerable<TransactionId> GetHistoryIds(long addressId)
        {
            var cmd = _getTxsByAddress;
            cmd.Reset();
            cmd.Parameters.Add(new SQLiteParameter("id", addressId));
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                Console.WriteLine($"Get IDs for address {addressId}");
                while (reader.Read())
                {
                    var id = (long) reader[0];
                    var order = _timestampIndex.GetTimestamp(id);
                    yield return new TransactionId
                    {
                        TxId = id,
                        Order = order,
                    };
                }
            }
        }

        private IEnumerable<TokenTransactionId> GetHistoryIds(Token token, long addressId)
        {
            var cmd = _getTokenTxsByAddress;
            cmd.Reset();
            cmd.Parameters.Add(new SQLiteParameter("tokens_id", token.Id));
            cmd.Parameters.Add(new SQLiteParameter("id", addressId));
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                Console.WriteLine($"Get IDs for address {addressId}");
                while (reader.Read())
                {
                    var id = (long)reader[0];
                    var amount = (string)reader[1];
                    var sender = _cache.Decode((long)reader[2]);
                    var receiver = _cache.Decode((long)reader[3]);
                    var order = _timestampIndex.GetTimestamp(id);

                    yield return new TokenTransactionId
                    {
                        TxId = id,
                        Order = order,
                        Amount = amount,
                        Sender = sender,
                        Receiver = receiver,
                    };
                }
            }
        }

        public TransactionRecord GetHistory(long txId, TimestampIndex.TransactionOrder order)
        {
            var cmd = _getHistoryByTxId;
            cmd.Reset();
            cmd.Parameters.Add(new SQLiteParameter("id", txId));
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (!reader.Read())
                    return null;
                var hash = (string) reader[0];
                var sender = _cache.Decode((long) reader[1]);
                var receiver = _cache.Decode((long) reader[2]);
                var value = (string) reader[3];
                var blockHash = (string) reader[4];
                var block = Blockchain.GetBlockByHash(blockHash);

                return new TransactionRecord
                {
                    Hash = hash,
                    Sender = sender,
                    Receiver = receiver,
                    Value = value,
                    BlockHash = blockHash,
                    BlockHeight = block.Height,
                    Order = order,
                };
            }
        }

        private TransactionRecord GetHistory(TransactionId id)
        {
            var cmd = _getHistoryByTxId;
            cmd.Reset();
            cmd.Parameters.Add(new SQLiteParameter("id", id.TxId));
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                return id.GetTransactionRecord(reader, _cache, Blockchain);
        }

        private List<TransactionRecord> TrimHistory(List<TransactionId> ids, long maxTxs, bool oldestFirst)
        {
            if (oldestFirst)
                ids.Sort((x, y) => TimestampIndex.TransactionOrder.Cmp(x.Order, y.Order));
            else
                ids.Sort((x, y) => -TimestampIndex.TransactionOrder.Cmp(x.Order, y.Order));
            var ret = ids
                .Take((int)maxTxs)
                .Select(GetHistory)
                .Where(x => x != null)
                .ToList();
            return ret;
        }

        public List<TransactionRecord> GetHistory(IEnumerable<string> addresses, long maxTxs, bool oldestFirst)
        {
            var ids = new List<TransactionId>();
            foreach (var id in MapAddresses(addresses))
                ids.AddRange(GetHistoryIds(id));
            return TrimHistory(ids, maxTxs, oldestFirst);
        }

        public List<TransactionRecord> GetHistory(Token token, IEnumerable<string> addresses, long maxTxs, bool oldestFirst)
        {
            var ids = new List<TransactionId>();
            foreach (var id in MapAddresses(addresses))
                ids.AddRange(GetHistoryIds(token, id));
            return TrimHistory(ids, maxTxs, oldestFirst);
        }

        private BigInteger GetAverageGasPriceForBlock(long id)
        {
            _readAverageGasPrice.Reset();
            _readAverageGasPrice.Parameters.Clear();
            _readAverageGasPrice.Parameters.Add(new SQLiteParameter("id", id));
            using (var reader = _readAverageGasPrice.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                if (!reader.Read())
                    return -1;
                var avg = (string) reader[0];
                if (avg != null)
                    return BigInteger.Parse(avg);
            }

            _readGasPrices.Reset();
            _readGasPrices.Parameters.Clear();
            _readGasPrices.Parameters.Add(new SQLiteParameter("blocks_id", id));
            BigInteger ret = 0;
            using (var reader = _readGasPrices.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                BigInteger sum = 0;
                int count = 0;
                while (reader.Read())
                {
                    sum += BigInteger.Parse((string) reader[0]);
                    count++;
                }
                ret = sum/count;
            }

            _writeAverageGasPrice.Reset();
            _writeAverageGasPrice.Parameters.Clear();
            _writeAverageGasPrice.Parameters.Add(new SQLiteParameter("average_gasprice", ret.ToString()));
            _writeAverageGasPrice.Parameters.Add(new SQLiteParameter("id", id));
            _writeAverageGasPrice.ExecuteNonQuery();

            return ret;
        }

        public BigInteger GetGasPrice()
        {
            BigInteger sum = 0;
            int count = 0;
            var height = Blockchain.Height;
            for (int i = 0; i < 60; i++)
            {
                var dbId = Blockchain.GetBlockByHeight(height - i).DbId;
                var n = GetAverageGasPriceForBlock(dbId);
                if (n < 0)
                    continue;
                sum += n;
                count++;
            }
            return sum / count;
        }

        private Dictionary<string, Token> GetTokens()
        {
            var ret = new Dictionary<string, Token>(StringComparer.InvariantCultureIgnoreCase);
            using (var cmd = new SQLiteCommand("select id, name, address from tokens;", _conn))
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                while (reader.Read())
                {
                    var id = (long) reader[0];
                    var name = (string) reader[1];
                    var address = (string) reader[2];
                    var addressId = _cache.Encode(address);
                    ret[address] = new Token
                    {
                        Id = id,
                        Name = name,
                        Address = address,
                        AddressId = addressId,
                    };
                }
            }
            return ret;
        }

        private Dictionary<string, Token> _tokens;

        public Dictionary<string, Token> Tokens => _tokens ?? (_tokens = GetTokens());

        public long? GetLatestBlockForToken(long tokenId)
        {
            long? ret = null;
            using (var cmd = new SQLiteCommand("select distinct blocks.hash from token_txs inner join txs on txs.id = token_txs.txs_id inner join blocks on blocks.id = txs.blocks_id where token_txs.tokens_id = @id;", _conn))
            {
                cmd.Parameters.Clear();
                cmd.Parameters.Add(new SQLiteParameter("id", tokenId));
                using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                {
                    while (reader.Read())
                    {
                        var hash = (string) reader[0];
                        var block = Blockchain.GetBlockByHash(hash);
                        if (block == null)
                        {
                            Debug.Assert(false);
                            continue;
                        }
                        if (ret == null || ret.Value < block.Height)
                            ret = block.Height;
                    }
                }
            }
            return ret;
        }

        public List<long> GetBlocksWithTransactions(long addressId, long minBlockId)
        {
            var ret = new List<long>();
            using (var cmd = new SQLiteCommand("select distinct blocks_id from txs where receiver = @receiver;", _conn))
            {
                cmd.Reset();
                cmd.Parameters.Clear();
                cmd.Parameters.Add(new SQLiteParameter("receiver", addressId));
                using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
                {
                    while (reader.Read())
                    {
                        var block = (long) reader[0];
                        if (block >= minBlockId)
                            ret.Add(block);
                    }
                }
            }
            return ret;
        }

        public List<Tuple<long, string>> GetTransactionsToAddressInBlock(long blockId, long addressId)
        {
            var ret = new List<Tuple<long, string>>();
            var cmd = _getTransactionsToAddressInBlock;
            cmd.Reset();
            cmd.Parameters.Clear();
            cmd.Parameters.Add(new SQLiteParameter("receiver", addressId));
            cmd.Parameters.Add(new SQLiteParameter("blocks_id", blockId));
            using (var reader = cmd.ExecuteReader(CommandBehavior.SequentialAccess))
            {
                while (reader.Read())
                {
                    var id = (long) reader[0];
                    var hash = (string) reader[1];
                    ret.Add(new Tuple<long, string>(id, hash));
                }
            }
            return ret;
        }

        public void AddTokenTransaction(long txId, long tokenId, int functionId, string sender, string receiver, string amount)
        {
            var cmd = _insertTokenTransaction;
            var senderId = _cache.Encode(sender);
            var receiverId = _cache.Encode(receiver);
            cmd.Reset();
            cmd.Parameters.Clear();
            cmd.Parameters.Add(new SQLiteParameter("txs_id", txId));
            cmd.Parameters.Add(new SQLiteParameter("tokens_id", tokenId));
            cmd.Parameters.Add(new SQLiteParameter("function_id", functionId));
            cmd.Parameters.Add(new SQLiteParameter("sender", senderId));
            cmd.Parameters.Add(new SQLiteParameter("receiver", receiverId));
            cmd.Parameters.Add(new SQLiteParameter("amount_wei", amount));
            cmd.ExecuteNonQuery();
        }

        public AbstractBlockchain.Block GetBlockByHeight(long height)
        {
            lock (_blockchainLock)
                return Blockchain.GetBlockByHeight(height);
        }

        public AbstractBlockchain.ChainReorganization TryAddNewBlock(string previousHash)
        {
            lock (_blockchainLock)
                return Blockchain.TryAddNewBlock(previousHash);
        }

        public long AddNewBlock(string hash, string previousHash, long dbId)
        {
            lock (_blockchainLock)
                return Blockchain.AddNewBlock(hash, previousHash, dbId);
        }
    }
}
