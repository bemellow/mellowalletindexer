using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EthereumIndex
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
                using (var index = new EthIndex(true))
                using (var service = new RestService(index))
                {
                    index.ContinueRunning = () => ContinueRunning;
                    service.Run(() => ContinueRunning);
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
