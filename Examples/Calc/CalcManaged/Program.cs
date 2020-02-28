using System;
using IPC.Managed;

namespace Calc.Managed
{
    static class Program
    {
        private static readonly TransportFactory _factory = new TransportFactory();

        static Program()
        {
            _factory.Register(System.Reflection.Assembly.Load("CalcInterop"));
        }

        static int Main(string[] args)
        {
            var address = "ipc://calc";

            switch (args.Length)
            {
                case 1:
                    if (args[0] == "--server")
                    {
                        Server.Run(_factory, address);
                        break;
                    }
                    else if (args[0] == "--client")
                    {
                        Client.Run(_factory, address);
                        break;
                    }
                    goto default;

                default:
                    Console.WriteLine("Pass --server or --client option.");
                    return 1;
            }

            return 0;
        }
    }
}
