using System;
using System.Threading;
using IPC.Managed;

namespace CalcManaged
{
    internal static class Server
    {
        public static void Run(TransportFactory factory, string address)
        {
            Console.WriteLine($"Hosting server at {address}");

            using (var transport = factory.Make<Calc.Managed.Request, Calc.Managed.Response>())
            using (var serversAccessor = transport.AcceptServers(address, (inMemory, outMemory) => new Service(outMemory).Invoke))
            {
                serversAccessor.Connected += (sender, args) =>
                    Console.WriteLine($"Connected: {args.Component.InputMemory.Name} -> {args.Component.OutputMemory.Name}");

                serversAccessor.Disconnected += (sender, args) =>
                    Console.WriteLine($"Disconnected: {args.Component.InputMemory.Name} -> {args.Component.OutputMemory.Name}");

                Console.WriteLine("Press Ctrl+C to exit.");

                var exit = new ManualResetEvent(false);
                Console.CancelKeyPress += (sender, args) => { args.Cancel = true; exit.Set(); };
                exit.WaitOne();

                Console.WriteLine("Exiting...");
            }
        }
    }
}
