using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using BitcoinLib.ExceptionHandling.Rpc;
using BitcoinLib.RPC.Connector;
using BitcoinLib.RPC.Specifications;
using BitcoinLib.Services.Coins.Bitcoin;

namespace NktBtcIndex
{
    class BtcRpc
    {
        private static readonly log4net.ILog Log = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);
        private RpcConnector _conn;
        public BtcRpc()
        {
            var config = Configuration.Get();
            _conn = new RpcConnector(new BitcoinService(config.Url, config.RpcUser, config.RpcPassword, string.Empty, 10));
        }

        public long GetLatestBlock()
        {
            int sleep = 1;
            for (int retry = 0;; retry++)
            {
                try
                {
                    return _conn.MakeRequest<long>(RpcMethods.getblockcount);
                }
                catch
                {
                    if (retry == 5)
                        throw;
                }
                Log.Warn($"Exception caught. Sleeping for {sleep} seconds and retrying...");
                Thread.Sleep(sleep * 1000);
                sleep *= 2;
            }
        }

        public string GetBlockHash(long height)
        {
            int sleep = 1;
            for (int retry = 0; ; retry++)
            {
                try
                {
                    return _conn.MakeRequest<string>(RpcMethods.getblockhash, new object[] { height });
                }
                catch
                {
                    if (retry == 5)
                        throw;
                }
                Log.Warn($"Exception caught. Sleeping for {sleep} seconds and retrying...");
                Thread.Sleep(sleep * 1000);
                sleep *= 2;
            }
        }

        public byte[] GetRawBlock(string hash)
        {
            int sleep = 1;
            for (int retry = 0; ; retry++)
            {
                try
                {
                    var s = _conn.MakeRequest<string>(RpcMethods.getblock, new object[] {hash, 0});
                    return Utility.HexStringToByteArray(s);
                }
                catch
                {
                    if (retry == 5)
                        throw;
                }
                Log.Warn($"Exception caught. Sleeping for {sleep} seconds and retrying...");
                Thread.Sleep(sleep * 1000);
                sleep *= 2;
            }
        }
    }
}
