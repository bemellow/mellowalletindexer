using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace LibraryTest
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                var parser = new BtcIndex(@"M:\bitcoin_index\btc.sqlite");
                var sw = new Stopwatch();
                while (true)
                {
                    var addresses = new List<string>();
                    string line;
                    while (true)
                    {
                        Console.Write($"{addresses.Count + 1}>");
                        line = Console.ReadLine();
                        if (string.IsNullOrEmpty(line))
                            break;
                        addresses.Add(line);
                    }
                    if (addresses.Count == 0)
                        break;

                    sw.Restart();
                    //var result = parser.GetTxs(addresses);
                    var result = parser.GetBalance(addresses);
                    sw.Stop();

                    //Console.WriteLine(result);
                    //Console.WriteLine($"Items {Newtonsoft.Json.JsonConvert.DeserializeObject<long[]>(result).Length}");
                    Console.WriteLine($"Items {Newtonsoft.Json.JsonConvert.DeserializeObject<long>(result)}");
                    Console.WriteLine($"Completed in {sw.ElapsedMilliseconds} ms.");
                }
            }
            catch (Exception e)
            {
                Console.Error.WriteLine(e.Message);
                Console.Error.WriteLine(e.StackTrace);
            }
        }
    }
}
