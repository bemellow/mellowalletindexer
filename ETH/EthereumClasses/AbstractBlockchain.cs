using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public abstract class AbstractBlockchain : IDisposable
    {
        public class ChainReorganizationBlock
        {
            public string Hash;
            public long DbId;
            public long Height;
        }

        public class ChainReorganization
        {
            public string BlockRequired;
            public List<ChainReorganizationBlock> BlocksToRevert;
        }

        public class Block
        {
            public string Hash;
            public string PreviousHash;
            public long Height = 0;
            public long DbId = 0;
            public long PreviousBlockId = 0;
        }

        protected Database _db;
        protected NetworkType _network;

        public AbstractBlockchain(Database db, NetworkType net)
        {
            _db = db;
            _network = net;
        }

        public abstract ChainReorganization TryAddNewBlock(string previousHash);
        public abstract long AddNewBlock(string hash, string previousHash, long dbId);
        protected abstract Block GetBlockByHeightInternal(long height);
        public abstract Block GetBlockByHash(string hash);

        public Block GetBlockByHeight(long height)
        {
            if (!_network.NetworkIsEthereum())
                height--;
            return GetBlockByHeightInternal(height);
        }

        public long Height
        {
            get
            {
                var ret = InternalHeight;
                return _network.NetworkIsEthereum() ? ret : ret + 1;
            }
        }

        public abstract string Head { get; }
        public abstract void Dispose();
        public abstract void RevertBlock(string hash);

        protected void UpdateDb()
        {
            var head = Head;
            if (head != null)
                _db.BlockchainHead = head;
        }

        protected abstract long InternalHeight { get; }
    }
}
