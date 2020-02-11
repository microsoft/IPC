using IPC.Managed;
using System;

namespace CalcManaged
{
    static class Program
    {
        private static readonly TransportFactory _factory = new TransportFactory();

        static Program()
        {
            _factory.Register(System.Reflection.Assembly.Load("CalcInterop"));
        }

        static void RunServer(string address)
        {
            Server.Run(_factory, address);
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
                    Console.WriteLine("Pass --server or --client options.");
                    return 1;
            }

            return 0;
        }
    }
}
