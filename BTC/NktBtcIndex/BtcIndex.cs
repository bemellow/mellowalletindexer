using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using BitcoinLib.RPC.Connector;
using BitcoinLib.RPC.Specifications;
using BitcoinLib.Services.Coins.Bitcoin;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NktBtcIndex;

namespace NktBtcIndex
{
    class BtcIndex : IDisposable
    {
        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr initialize_index([MarshalAs(UnmanagedType.LPArray)] byte[] db_path, bool testnet);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void destroy_index(IntPtr parser);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_get_utxo(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_get_utxo_insight(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_get_balance(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_get_balances(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_get_history(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] addresses);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern long index_get_blockchain_height(IntPtr dll);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_push_new_block(IntPtr dll, [MarshalAs(UnmanagedType.LPArray)] byte[] data, IntPtr data_size);

        [DllImport("libbtcindex", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr index_get_fees(IntPtr dll);

        private static readonly log4net.ILog Log = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        private IntPtr _index = IntPtr.Zero;
        private Thread _updateThread;
        private bool _continueRunning = true;
        private BtcRpc _rpc;

        public BtcIndex(bool update, bool testnet)
        {
            var config = Configuration.Get();
            Log.Info("Initializing index...");
            _index = initialize_index(Utility.StringToUtf8(config.DbPath), testnet);
            Log.Info("Index initialized.");
            if (_index == IntPtr.Zero)
                throw new Exception("Failed to initialize parser");

            _rpc = new BtcRpc();

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
                Log.Info("Stopping update thread.");
                _continueRunning = false;
                _updateThread.Join();
                _updateThread = null;
            }
            if (_index != IntPtr.Zero)
            {
                destroy_index(_index);
                _index = IntPtr.Zero;
            }
        }
        
        public string GetUtxo(string jsonAddresses)
        {
            return Utility.Utf8ToString(index_get_utxo(_index, Utility.StringToUtf8(jsonAddresses)));
        }

        public string GetUtxoInsight(string jsonAddresses)
        {
            return Utility.Utf8ToString(index_get_utxo_insight(_index, Utility.StringToUtf8(jsonAddresses)));
        }

        public string GetBalance(IEnumerable<string> addresses)
        {
            return GetBalance(Newtonsoft.Json.JsonConvert.SerializeObject(addresses));
        }

        public string GetBalance(string jsonAddresses)
        {
            return Utility.Utf8ToString(index_get_balance(_index, Utility.StringToUtf8(jsonAddresses)));
        }

        public string GetBalances(string jsonAddresses)
        {
            return Utility.Utf8ToString(index_get_balances(_index, Utility.StringToUtf8(jsonAddresses)));
        }

        public string GetHistory(string jsonParams)
        {
            return Utility.Utf8ToString(index_get_history(_index, Utility.StringToUtf8(jsonParams)));
        }

        public long GetLatestIndexedBlock()
        {
            return index_get_blockchain_height(_index);
        }

        public class PushNewBlockResult
        {
            [JsonProperty("block_required")]
            public string BlockRequired;
            [JsonProperty("blocks_reverted")]
            public long BlocksReverted;
            [JsonProperty("db_id")]
            public long DbId;
            [JsonProperty("new_height")]
            public long NewHeight;
        }

        public PushNewBlockResult PushNewBlock(byte[] rawBlock)
        {
            IntPtr length;
            if (IntPtr.Size == 4)
                length = (IntPtr) rawBlock.Length;
            else if (IntPtr.Size == 8)
                length = (IntPtr) rawBlock.LongLength;
            else
                throw new Exception("Unsupported platform. Must be 32- or 64-bit.");
            var jsonString = Utility.Utf8ToString(index_push_new_block(_index, rawBlock, length));
            if (jsonString == null)
                throw new Exception("libbtcindex threw an error.");
            return JsonConvert.DeserializeObject<PushNewBlockResult>(jsonString);
        }

        public string GetFees()
        {
            return Utility.Utf8ToString(index_get_fees(_index));
        }

        private long UpdateToHash(string hash)
        {
            var result = PushNewBlock(_rpc.GetRawBlock(hash));
            if (result.BlockRequired != null)
            {
                UpdateToHash(result.BlockRequired);
                result = PushNewBlock(_rpc.GetRawBlock(hash));
            }
            if (result.BlocksReverted != 0)
                Log.Info($"Blocks reverted: {result.BlocksReverted}");
            return result.NewHeight;
        }

        private void DoInitialUpdate()
        {
            Log.Info("Doing initial update.");
            var latestBlock = GetLatestIndexedBlock();
            var latestBlockInBlockChain = _rpc.GetLatestBlock();
            var sw = new Stopwatch();
            while (_continueRunning && latestBlock < latestBlockInBlockChain)
            {
                Log.Info($"Updating to block height {latestBlock + 1} ({latestBlockInBlockChain - latestBlock} remaining).");
                sw.Restart();
                latestBlock = UpdateToHash(_rpc.GetBlockHash(latestBlock + 1));
                sw.Stop();
                Log.Info($"Elapsed: {sw.ElapsedMilliseconds} ms");
                latestBlockInBlockChain = _rpc.GetLatestBlock();
            }
            if (_continueRunning)
                Log.Info("Initial update complete.");
        }

        private void UpdateThreadFunction()
        {
            try
            {
                Log.Info("Update thread started.");
                Thread.CurrentThread.Name = "NktBtcIndex update thread";
                DoInitialUpdate();
                var latestBlock = GetLatestIndexedBlock();
                var sw = new Stopwatch();
                while (_continueRunning)
                {
                    Thread.Sleep(1000);
                    var latestBlockInBlockChain = _rpc.GetLatestBlock();
                    while (latestBlock < latestBlockInBlockChain)
                    {
                        Log.Info($"Updating to block height {latestBlock + 1}.");
                        sw.Restart();
                        latestBlock = UpdateToHash(_rpc.GetBlockHash(latestBlockInBlockChain));
                        sw.Stop();
                        Log.Info($"Elapsed: {sw.ElapsedMilliseconds} ms");
                    }
                }
            }
            catch (Exception e)
            {
                int level = 0;
                var oldE = e;
                while (e != null)
                {
                    var arrow = level == 0 ? "" : new String('-', level) + ">";
                    Log.Error($"{arrow}{e.Message} ({e.GetType().Name})");
                    level++;
                    e = e.InnerException;
                }
                Log.Error(oldE.StackTrace);
                Environment.Exit(-1);
            }
        }
    }
}
