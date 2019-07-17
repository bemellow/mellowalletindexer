using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EthereumClasses
{
    class CppBlockchain : AbstractBlockchain
    {
        private struct Sha256
        {
            public byte Byte00;
            public byte Byte01;
            public byte Byte02;
            public byte Byte03;
            public byte Byte04;
            public byte Byte05;
            public byte Byte06;
            public byte Byte07;
            public byte Byte08;
            public byte Byte09;
            public byte Byte10;
            public byte Byte11;
            public byte Byte12;
            public byte Byte13;
            public byte Byte14;
            public byte Byte15;
            public byte Byte16;
            public byte Byte17;
            public byte Byte18;
            public byte Byte19;
            public byte Byte20;
            public byte Byte21;
            public byte Byte22;
            public byte Byte23;
            public byte Byte24;
            public byte Byte25;
            public byte Byte26;
            public byte Byte27;
            public byte Byte28;
            public byte Byte29;
            public byte Byte30;
            public byte Byte31;
        }

        private static byte ToByte(char c)
        {
            switch (c)
            {
                case '0':
                    return 0;
                case '1':
                    return 1;
                case '2':
                    return 2;
                case '3':
                    return 3;
                case '4':
                    return 4;
                case '5':
                    return 5;
                case '6':
                    return 6;
                case '7':
                    return 7;
                case '8':
                    return 8;
                case '9':
                    return 9;
                case 'a':
                    return 10;
                case 'b':
                    return 11;
                case 'c':
                    return 12;
                case 'd':
                    return 13;
                case 'e':
                    return 14;
                case 'f':
                    return 15;
            }
            throw new FormatException($"Invalid hex character: {c}");
        }

        private static Sha256 ToSha256(string s)
        {
            var original = s;
            s = s.ToLower();
            if (s.StartsWith("0x"))
                s = s.Substring(2);
            if (s.Length != 64)
                throw new FormatException($"SHA-256 string must have exactly 64 hex digits: {original}");
            var temp = new byte[32];
            for (int i = 0; i < 64; i++)
            {
                var b = ToByte(s[i]);
                if (i%2 == 0)
                {
                    temp[i/2] = (byte)(b << 4);
                }
                else
                {
                    temp[i / 2] |= b;
                }
            }
            return new Sha256
            {
                Byte00 = temp[00],
                Byte01 = temp[01],
                Byte02 = temp[02],
                Byte03 = temp[03],
                Byte04 = temp[04],
                Byte05 = temp[05],
                Byte06 = temp[06],
                Byte07 = temp[07],
                Byte08 = temp[08],
                Byte09 = temp[09],

                Byte10 = temp[10],
                Byte11 = temp[11],
                Byte12 = temp[12],
                Byte13 = temp[13],
                Byte14 = temp[14],
                Byte15 = temp[15],
                Byte16 = temp[16],
                Byte17 = temp[17],
                Byte18 = temp[18],
                Byte19 = temp[19],

                Byte20 = temp[20],
                Byte21 = temp[21],
                Byte22 = temp[22],
                Byte23 = temp[23],
                Byte24 = temp[24],
                Byte25 = temp[25],
                Byte26 = temp[26],
                Byte27 = temp[27],
                Byte28 = temp[28],
                Byte29 = temp[29],

                Byte30 = temp[30],
                Byte31 = temp[31],
            };
        }

        private static string ToString(Sha256 sha256)
        {
            var temp = new byte[32];

            temp[00] = sha256.Byte00;
            temp[01] = sha256.Byte01;
            temp[02] = sha256.Byte02;
            temp[03] = sha256.Byte03;
            temp[04] = sha256.Byte04;
            temp[05] = sha256.Byte05;
            temp[06] = sha256.Byte06;
            temp[07] = sha256.Byte07;
            temp[08] = sha256.Byte08;
            temp[09] = sha256.Byte09;

            temp[10] = sha256.Byte10;
            temp[11] = sha256.Byte11;
            temp[12] = sha256.Byte12;
            temp[13] = sha256.Byte13;
            temp[14] = sha256.Byte14;
            temp[15] = sha256.Byte15;
            temp[16] = sha256.Byte16;
            temp[17] = sha256.Byte17;
            temp[18] = sha256.Byte18;
            temp[19] = sha256.Byte19;

            temp[20] = sha256.Byte20;
            temp[21] = sha256.Byte21;
            temp[22] = sha256.Byte22;
            temp[23] = sha256.Byte23;
            temp[24] = sha256.Byte24;
            temp[25] = sha256.Byte25;
            temp[26] = sha256.Byte26;
            temp[27] = sha256.Byte27;
            temp[28] = sha256.Byte28;
            temp[29] = sha256.Byte29;

            temp[30] = sha256.Byte30;
            temp[31] = sha256.Byte31;

            return "0x" + BitConverter.ToString(temp).Replace("-", string.Empty).ToLower();
        }

        private struct CppChainReorganizationBlock
        {
	        public Sha256 Hash;
            public long DbId;
            public long Height;
        };

        private struct CppChainReorganization
        {
            public bool BlockRequiredHasValue;
            public Sha256 BlockRequired;
            public long RevertedBlockCount;
            public IntPtr RevertedBlocks;
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate bool GetBlockDataCallback(ref long id, ref Sha256 hash, ref Sha256 previousHash);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate bool GetBlockchainHead(ref Sha256 hash);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr create_blockchain(GetBlockDataCallback c1, GetBlockchainHead c2);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void destroy_blockchain(IntPtr blockchain);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr try_add_new_block(IntPtr blockchain, [MarshalAs(UnmanagedType.LPArray)] byte[] previousHash);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void release_string(IntPtr blockchain, IntPtr s);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern long add_new_block(IntPtr blockchain, [MarshalAs(UnmanagedType.LPArray)] byte[] hash, [MarshalAs(UnmanagedType.LPArray)] byte[] previousHash, long dbId);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr get_block_by_height(IntPtr blockchain, long height);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr get_block_by_hash(IntPtr blockchain, [MarshalAs(UnmanagedType.LPArray)] byte[] hash);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern long get_height(IntPtr blockchain);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr get_blockchain_head(IntPtr blockchain);

        [DllImport("libcpphelper", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern int revert_block(IntPtr blockchain, [MarshalAs(UnmanagedType.LPArray)] byte[] hash);

        private IntPtr _blockchain;

        public CppBlockchain(Database db) : base(db, Configuration.Get().Network)
        {
            var blocks = _db.GetAllBlocks().GetEnumerator();
            _blockchain = create_blockchain(
                (ref long id, ref Sha256 hash, ref Sha256 previousHash) =>
                {
                    if (!blocks.MoveNext())
                        return false;
                    var block = blocks.Current;
                    try
                    {
                        id = block.Id;
                        hash = ToSha256(block.Hash);
                        previousHash = ToSha256(block.PreviousHash);
                    }
                    catch (Exception e)
                    {
                        throw new Exception($"Error while processing block {block.Id}, {block.Hash}, {block.PreviousHash}", e);
                    }
                    return true;
                },
                (ref Sha256 hash) =>
                {
                    var head = _db.BlockchainHead;
                    if (head == null)
                        return false;
                    hash = ToSha256(head);
                    return false;
                }
            );

            //var sb = new StringBuilder();
            //long count = 0;
            //foreach (var block in _db.GetAllBlocks())
            //{
            //    sb.Append(' ');
            //    sb.Append(block.Id);
            //    sb.Append(' ');
            //    sb.Append(block.Hash);
            //    sb.Append(' ');
            //    sb.Append(block.PreviousHash);
            //    count++;
            //}
            //{
            //    var head = _db.BlockchainHead;
            //    if (head != null)
            //    {
            //        sb.Append(" 1 ");
            //        sb.Append(head);
            //    }
            //    else
            //        sb.Append(" 0");
            //}
            //_blockchain = create_blockchain((count.ToString() + sb).StringToUtf8());

            if (_blockchain == IntPtr.Zero)
                throw new Exception("Could not initialize CppBlockchain.");
            UpdateDb();
        }

        public override void Dispose()
        {
            if (_blockchain != IntPtr.Zero)
            {
                destroy_blockchain(_blockchain);
                _blockchain = IntPtr.Zero;
            }
        }

        private string NativeStringToString(IntPtr p)
        {
            if (p == IntPtr.Zero)
                return null;
            var ret = Utility.Utf8ToString(p);
            release_string(_blockchain, p);
            return ret;
        }

        private static char[] Space = { ' ' };

        public override ChainReorganization TryAddNewBlock(string previousHash)
        {
            lock (this)
            {
                var result =
                    NativeStringToString(try_add_new_block(_blockchain, previousHash.StringToUtf8()))
                        .Split(Space)
                        .ToArray();

                var ret = new ChainReorganization();
                long offset = 0;
                int blockRequired = Convert.ToInt32(result[offset++]);
                if (blockRequired != 0)
                {
                    ret.BlockRequired = result[offset++];
                }
                var blocksToRevert = Convert.ToInt32(result[offset++]);
                ret.BlocksToRevert = new List<ChainReorganizationBlock>(blocksToRevert);
                for (int i = 0; i < blocksToRevert; i++)
                {
                    var crb = new ChainReorganizationBlock();
                    crb.Hash = result[offset++];
                    crb.DbId = Convert.ToInt64(result[offset++]);
                    crb.Height = Convert.ToInt64(result[offset++]);
                    ret.BlocksToRevert.Add(crb);
                }
                ret.BlocksToRevert = ret.BlocksToRevert.OrderByDescending(x => x.Height).ToList();
                return ret;
            }
        }
        
        public override long AddNewBlock(string hash, string previousHash, long dbId)
        {
            lock (this)
            {
                var cr = TryAddNewBlock(previousHash);
                if (cr.BlockRequired != null)
                    throw new Exception(
                        "Incorrect usage. Call Blockchain.TryAddNewBlock() first and add blocks in the correct order.");
                var ret = add_new_block(_blockchain, hash.StringToUtf8(), previousHash.StringToUtf8(), dbId);
                UpdateDb();
                return ret;
            }
        }

        private Block ParseBlock(IntPtr p)
        {
            if (p == IntPtr.Zero)
                return null;
            var split = NativeStringToString(p).Split(Space).ToArray();
            long offset = 0;
            var ret = new Block();
            ret.Hash = split[offset++];
            ret.PreviousHash = split[offset++];
            ret.Height = Convert.ToInt64(split[offset++]);
            ret.DbId = Convert.ToInt64(split[offset++]);
            ret.PreviousBlockId = Convert.ToInt64(split[offset++]);
            return ret;
        }

        protected override Block GetBlockByHeightInternal(long height)
        {
            lock (this)
                return ParseBlock(get_block_by_height(_blockchain, height));
        }

        public override Block GetBlockByHash(string hash)
        {
            lock (this)
                return ParseBlock(get_block_by_hash(_blockchain, hash.StringToUtf8()));
        }

        protected override long InternalHeight
        {
            get
            {
                lock (this)
                    return get_height(_blockchain);
            }
        }

        public override string Head
        {
            get
            {
                lock (this)
                    return NativeStringToString(get_blockchain_head(_blockchain));
            }
        }

        public override void RevertBlock(string hash)
        {
            var result = revert_block(_blockchain, hash.StringToUtf8());
            if (result == 1)
                throw new Exception($"{GetType().Name}: Incorrect usage. Trying to remove block from an empty blockchain.");
            if (result == 2)
                throw new Exception($"{GetType().Name}: Incorrect usage. Trying to remove block other than the head.");
            UpdateDb();
        }
    }
}
