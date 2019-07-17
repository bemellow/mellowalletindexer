using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using EthereumIndex;
using Nethereum.Hex.HexTypes;
using Nethereum.RPC.Eth.DTOs;
using Nethereum.Web3;
using Org.BouncyCastle.Math;
using SHA1 = System.Security.Cryptography.SHA1;

namespace EthereumClasses
{
    public static class Utility
    {
        public static List<byte> ParseHexString(string code)
        {
            var ret = new List<byte>();
            if (string.IsNullOrEmpty(code))
                return ret;
            if (code == "0x" || code == "0X")
                return ret;
            if (code.StartsWith("0x"))
                code = code.Substring(2);
            if (code.Length % 2 != 0)
                return null;
            for (int i = 0; i < code.Length; i += 2)
            {
                var b = byte.Parse(code.Substring(i, 2), System.Globalization.NumberStyles.HexNumber);
                ret.Add(b);
            }
            return ret;
        }

        public static void ForEach<T>(this IEnumerable<T> xs, Action<T> f)
        {
            foreach (var x in xs)
                f(x);
        }

        public static HashSet<T> ToSet<T>(this IEnumerable<T> xs)
        {
            var ret = new HashSet<T>();
            foreach (var x in xs)
                ret.Add(x);
            return ret;
        }

        public static string ToPaddedString(this BigInteger hex, int minLength)
        {
            var ret = hex.ToString(16);
            return "0x" + new string('0', Math.Max(minLength - ret.Length, 0)) + ret;
        }

        public static string ToEthereumAddress(this BigInteger hex)
        {
            return hex.ToPaddedString(40);
        }

        public static string ToEthereumHash(this BigInteger hex)
        {
            return hex.ToPaddedString(64);
        }

        public static BigInteger HexStringToBigInteger(this string hex)
        {
            if (hex.StartsWith("0x", StringComparison.InvariantCultureIgnoreCase))
                hex = hex.Substring(2);
            return new BigInteger(hex, 16);
        }

        private static BigInteger _two = new BigInteger("2");

        public static ulong UlongValue(this BigInteger big)
        {
            if (big.SignValue < 0)
                throw new Exception("Can't convert negative to ulong.");
            if (big.ShiftRight(64).SignValue > 0)
                throw new OverflowException("BigInteger >= 2^64");
            ulong ret = 0;
            for (int i = 0; i < 64; i++)
                ret |= (ulong)big.ShiftRight(i).Mod(_two).IntValue << i;
            return ret;
        }

        public static string ComputeCodeHash(this byte[] code)
        {
            var sha1 = SHA1.Create();
            sha1.TransformFinalBlock(code, 0, code.Length);
            var digest = sha1.Hash;
            return BitConverter.ToString(digest).Replace("-", "").ToLower();
        }

        public static long ToLong(this HexBigInteger n)
        {
            return Convert.ToInt64(n.Value.ToString());
        }

        public static byte[] StringToUtf8(this string s)
        {
            var tempList = Encoding.UTF8.GetBytes(s).ToList();
            tempList.Add(0);
            return tempList.ToArray();
        }

        public static string Utf8ToString(IntPtr ptr)
        {
            int size = 0;
            while (Marshal.ReadByte(ptr, size) != 0)
                size++;
            var buffer = new byte[size];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            return Encoding.UTF8.GetString(buffer);
        }

        public static System.Numerics.BigInteger ToBigInteger(this HexBigInteger h)
        {
            var s = h.HexValue.ToLower();
            int i = 0;
            if (s.StartsWith("0x"))
                i = 2;
            System.Numerics.BigInteger ret = 0;
            foreach (var c in s.Skip(i))
            {
                ret <<= 4;
                int n = 0;
                switch (c)
                {
                    case '0':
                        n = 0;
                        break;
                    case '1':
                        n = 1;
                        break;
                    case '2':
                        n = 2;
                        break;
                    case '3':
                        n = 3;
                        break;
                    case '4':
                        n = 4;
                        break;
                    case '5':
                        n = 5;
                        break;
                    case '6':
                        n = 6;
                        break;
                    case '7':
                        n = 7;
                        break;
                    case '8':
                        n = 8;
                        break;
                    case '9':
                        n = 9;
                        break;
                    case 'a':
                        n = 10;
                        break;
                    case 'b':
                        n = 11;
                        break;
                    case 'c':
                        n = 12;
                        break;
                    case 'd':
                        n = 13;
                        break;
                    case 'e':
                        n = 14;
                        break;
                    case 'f':
                        n = 15;
                        break;
                }
                ret += n;
            }
            return ret;
        }

        public static long ToLong(this System.Numerics.BigInteger n)
        {
            return Convert.ToInt64(n.ToString());
        }

        public static long ByzantiumForkBlockNumber(NetworkType network)
        {
            switch (network)
            {
                case NetworkType.Ethereum:
                    return 4370000;
                case NetworkType.Ropsten:
                    return 1700000;
                default:
                    throw new ArgumentOutOfRangeException(nameof(network), network, null);
            }
        }

        public static bool NetworkIsEthereum(this NetworkType net)
        {
            return net == NetworkType.Ethereum || net == NetworkType.Ropsten;
        }

        public static bool TransactionSucceeded(Transaction transaction, TransactionReceipt receipt, NetworkType network)
        {
            if (network.NetworkIsEthereum())
            {
                if (receipt == null)
                    throw new NullReceiptException();
                if (transaction.BlockNumber.Value.ToLong() < ByzantiumForkBlockNumber(network))
                    return receipt.GasUsed.Value < transaction.Gas;
                if (receipt.Status == null)
                    return false;
                return receipt.Status.Value.ToLong() == 1;
            }
            return true;
        }

        public static T[] EnumerateEnum<T>()
        {
            return (T[]) Enum.GetValues(typeof (T));
        }
    }
}
