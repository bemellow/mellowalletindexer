using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.Configuration;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    public class Blockchain : AbstractBlockchain
    {
        public class HeadCandidate : Block
        {
            public HeadCandidate Previous;
            public bool HasChildren;
        }

        private class BlocksTemp
        {
            public List<HeadCandidate> Blocks = new List<HeadCandidate>();
            public Dictionary<string, HeadCandidate> BlockMap = new Dictionary<string, HeadCandidate>(StringComparer.InvariantCultureIgnoreCase);
        }
        
        public delegate HeadCandidate HeadSelector(List<HeadCandidate> list);

        private List<Block> _blockchain;
        private Dictionary<string, int> _blockMap = new Dictionary<string, int>(StringComparer.InvariantCultureIgnoreCase);

        public Blockchain(Database db, HeadSelector selector = null) : base(db, Configuration.Get().Network)
        {
            var blocks = AssembleBlockchain(db, selector);
            _blockchain = new List<Block>(blocks.Count);
            foreach (var block in blocks)
            {
                var height = _blockchain.Count;
                var block2 = new Block
                {
                    Hash = block.Hash,
                    PreviousHash = block.PreviousHash,
                    Height = height,
                    DbId = block.DbId,
                    PreviousBlockId = block.PreviousBlockId
                };
                _blockMap[block2.Hash] = height;
                _blockchain.Add(block2);
            }
            UpdateDb();
        }

        public override void Dispose()
        {
        }

        protected override long InternalHeight => _blockchain.Count - 1;

        private static List<HeadCandidate> AssembleBlockchain(Database db, HeadSelector selector)
        {
            var blocks = BuildBlocks(db);

            var ret = new List<HeadCandidate>(blocks.Blocks.Count);
            if (blocks.Blocks.Count == 0)
                return ret;

            for (var current = SelectHead(db, blocks, selector); current != null; current = current.Previous)
            {
                ret.Add(current);
                var back = ret[ret.Count - 1];
                if (current.Previous != null)
                    back.PreviousBlockId = current.Previous.DbId;
                else
                    back.PreviousBlockId = long.MaxValue;
            }
            ret.Reverse();
            return ret;
        }

        private static HeadCandidate SelectHead(Database db, BlocksTemp blocks, HeadSelector selector)
        {
            var heads = blocks.Blocks.Where(block => !block.HasChildren).ToList();
            if (heads.Count == 0)
                throw new Exception("Invalid blockchain. Circular topologies are invalid.");

            if (heads.Count == 1)
                return heads[0];

            var hash = db.BlockchainHead;
            if (hash == null)
            {
                if (selector == null)
                    throw new Exception("Can't read blockchain head from DB.");
                hash = selector(heads).Hash;
            }

            HeadCandidate ret;
            if (!blocks.BlockMap.TryGetValue(hash, out ret))
                throw new Exception(
                    $"DB states that the blockchain head is {hash}, but no such block exists in the DB.");
            return ret;
        }

        private static BlocksTemp BuildBlocks(Database db)
        {
            var ret = new BlocksTemp();
            foreach (var block in db.GetAllBlocks())
            {
                var candidate = new HeadCandidate
                {
                    DbId = block.Id,
                    Hash = block.Hash,
                    PreviousHash = block.PreviousHash
                };
                ret.Blocks.Add(candidate);
                ret.BlockMap[block.Hash] = candidate;
            }
            foreach (var block in ret.Blocks)
            {
                HeadCandidate previous;
                if (!ret.BlockMap.TryGetValue(block.PreviousHash, out previous))
                    continue;
                block.Previous = previous;
                block.Previous.HasChildren = true;
            }
            return ret;
        }
        
        public override ChainReorganization TryAddNewBlock(string previousHash)
        {
            var ret = new ChainReorganization();
            if (_blockchain.Count == 0)
                return ret;
            int height;
            if (!_blockMap.TryGetValue(previousHash, out height))
            {
                ret.BlockRequired = previousHash;
                return ret;
            }
            var firstReverted = height + 1;
            ret.BlocksToRevert = new List<ChainReorganizationBlock>(_blockchain.Count - firstReverted);
            for (var i = firstReverted; i < _blockchain.Count; i++)
            {
                var block = _blockchain[i];
                var crb = new ChainReorganizationBlock
                {
                    Hash = block.Hash,
                    Height = block.Height,
                    DbId = block.DbId
                };
                ret.BlocksToRevert.Add(crb);
            }
            return ret;
        }

        public override long AddNewBlock(string hash, string previousHash, long dbId)
        {
            var cr = TryAddNewBlock(previousHash);
            if (cr.BlockRequired != null)
                throw new Exception("Incorrect usage. Call Blockchain.TryAddNewBlock() first and add blocks in the correct order.");
            var previousHeight = _blockchain.Count;
            if (cr.BlocksToRevert.Count > 0)
                previousHeight = (int)cr.BlocksToRevert[0].Height;
            foreach (var crb in cr.BlocksToRevert)
                _blockMap.Remove(GetBlockByHeight(crb.Height).Hash);
            var block = new Block();
            block.Hash = hash;
            block.PreviousHash = previousHash;
            block.Height = previousHeight;
            block.DbId = dbId;
            if (_blockchain.Count >= previousHeight + 1)
            {
                if (_blockchain.Count > previousHeight + 1)
                    _blockchain = _blockchain.Take(previousHeight + 1).ToList();
                _blockchain[previousHeight] = block;
            }
            else
            {
                Debug.Assert(_blockchain.Count == previousHeight);
                _blockchain.Add(block);
            }
            _blockMap[hash] = previousHeight;
            return block.Height;
        }

        protected override Block GetBlockByHeightInternal(long height)
        {
            if (height < 0 || height >= _blockchain.Count)
                return null;
            return _blockchain[(int)height];
        }

        public override Block GetBlockByHash(string hash)
        {
            int height;
            if (!_blockMap.TryGetValue(hash, out height))
                return null;
            return GetBlockByHeight(height);
        }

        public override string Head => _blockchain.Count == 0 ? null : _blockchain[_blockchain.Count - 1].Hash;

        public override void RevertBlock(string hash)
        {
            if (_blockchain.Count < 1)
                throw new Exception($"{GetType().Name}: Incorrect usage. Trying to remove block from an empty blockchain.");
            if (StringComparer.InvariantCultureIgnoreCase.Compare(_blockchain[_blockchain.Count - 1].Hash, hash) != 0)
                throw new Exception($"{GetType().Name}: Incorrect usage. Trying to remove block other than the head.");
            _blockMap.Remove(hash);
            _blockchain.RemoveAt(_blockchain.Count - 1);
            UpdateDb();
        }

    }
}
