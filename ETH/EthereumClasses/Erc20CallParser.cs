using System;
using System.Collections.Generic;
using System.Linq;
using Nethereum.Hex.HexTypes;

namespace EthereumClasses
{
    public class Erc20CallParser : ContractCallParser
    {
        private static List<Tuple<string, int>> _list = new List<Tuple<string, int>>
        {
            new Tuple<string, int>("transfer(address,uint256)", (int)Erc20Function.Transfer),
            new Tuple<string, int>("transferFrom(address,address,uint256)", (int)Erc20Function.TransferFrom),
            new Tuple<string, int>("approve(address)", (int)Erc20Function.Approve),
            new Tuple<string, int>("approve(address,uint256)", (int)Erc20Function.ApproveWithAmount),
        };

        private HashSet<Erc20Function> _functionsToProcess;

        public Erc20CallParser(HashSet<Erc20Function> functionsToProcess = null) : base(_list)
        {
            _functionsToProcess = functionsToProcess ?? new HashSet<Erc20Function>(Utility.EnumerateEnum<Erc20Function>());
        }

        private static string BufferToString(byte[] input, int start, int length)
        {
            return BitConverter.ToString(input, start, length).Replace("-", string.Empty).ToLower();
        }

        private static string ReadAddress(byte[] input, int offset)
        {
            return "0x" + BufferToString(input, offset + 12, 20);
        }

        private static string ReadNumber(byte[] input, int offset)
        {
            return new HexBigInteger("0x" + BufferToString(input, offset, 32)).Value.ToString();
        }

        public override ContractCall ParseInput(byte[] input, string sender, string receiver)
        {
            if (input.Length < 4)
                return null;
            var key = BufferToString(input, 0, 4);
            int function;
            if (!_map.TryGetValue(key, out function))
                return null;
            var efunction = (Erc20Function) function;
            if (!_functionsToProcess.Contains(efunction))
                return null;
            var ret = new Erc20Call();
            ret.Function = function;
            ret.Parameters = new List<string>();
            switch (efunction)
            {
                case Erc20Function.Transfer:
                    ret.Sender = sender;
                    ret.Receiver = ReadAddress(input, 4 + 32 * 0);
                    ret.Value = ReadNumber(input, 4 + 32 * 1);
                    ret.Parameters.Add(ret.Receiver);
                    ret.Parameters.Add(ret.Value);
                    break;
                case Erc20Function.TransferFrom:
                    ret.Sender = ReadAddress(input, 4 + 32 * 0);
                    ret.Parameters.Add(ret.Sender);
                    ret.Receiver = ReadAddress(input, 4 + 32 * 0);
                    ret.Value = ReadNumber(input, 4 + 32 * 2);
                    ret.Parameters.Add(ret.Receiver);
                    ret.Parameters.Add(ret.Value);
                    break;
                case Erc20Function.Approve:
                    ret.Receiver = ReadAddress(input, 4 + 32 * 0);
                    ret.Parameters.Add(ret.Receiver);
                    break;
                case Erc20Function.ApproveWithAmount:
                    ret.Receiver = ReadAddress(input, 4 + 32 * 0);
                    ret.Value = ReadNumber(input, 4 + 32 * 1);
                    ret.Parameters.Add(ret.Receiver);
                    ret.Parameters.Add(ret.Value);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
            return ret;
        }
    }
}
