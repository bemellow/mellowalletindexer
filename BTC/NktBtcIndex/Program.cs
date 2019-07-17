using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace NktBtcIndex
{
    class Program
    {
        private static readonly log4net.ILog Log = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        private static bool ContinueRunning = true;

        static void Main(string[] args)
        {
            try
            {
                Console.CancelKeyPress += Stop;
                using (var index = new BtcIndex(true, Configuration.Get().Testnet))
                using (var service = new RestService(index))
                {
                    service.Run(() => ContinueRunning);
                }
            }
            catch (Exception e)
            {
                Log.Error(e);
            }
        }

        private static void Stop(object sender, ConsoleCancelEventArgs e)
        {
            e.Cancel = true;
            Console.WriteLine("Ctrl+C caught.");
            ContinueRunning = false;
        }
    }
}
