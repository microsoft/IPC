using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using IPC.Managed;

namespace CalcManaged
{
    internal static class Client
    {
        private static double NextDouble(this Random random, double min, double max)
        {
            return random.NextDouble() * (max - min) + min;
        }

        public static void Run(TransportFactory factory, string address)
        {
            Console.WriteLine("Press Ctrl+C to exit.");
            var exit = new ManualResetEvent(false);
            Console.CancelKeyPress += (sender, args) => { args.Cancel = true; exit.Set(); };

            Console.WriteLine($"Connecting to {address}");

            using (var transport = factory.Make<Calc.Managed.Request, Calc.Managed.Response>())
            using (var clientAccessor = transport.ConnectClient(address, true))
            {
                clientAccessor.Error += (sender, args) => Console.WriteLine($"IPC: {args.Exception.Message}");

                var random = new Random();
                IClient<Calc.Managed.Request, Calc.Managed.Response> client = null;

                while (!exit.WaitOne(TimeSpan.FromSeconds(1)))
                {
                    if (client == null)
                    {
                        try
                        {
                            client = clientAccessor.Client;
                        }
                        catch (System.Exception e)
                        {
                            Console.WriteLine($"Error: {e.Message}");
                            continue;
                        }

                        Console.WriteLine($"Connected: {client.InputMemory.Name} -> {client.OutputMemory.Name}");

                        client.Closed += (sender, args) =>
                            Console.WriteLine($"Disconnected: {client.InputMemory.Name} -> {client.OutputMemory.Name}");
                    }

                    var request = new Calc.Managed.Request
                    {
                        X = (float)random.NextDouble(1.0, 99.0),
                        Y = (float)random.NextDouble(1.0, 99.0),
                        Op = (Calc.Managed.Operation)random.Next(0, 3)
                    };

                    Calc.Managed.Response response;

                    var stopwatch = new Stopwatch();
                    stopwatch.Start();

                    try
                    {
                        response = client.InvokeAsync(request).Result;
                    }
                    catch (System.Exception e)
                    {
                        Console.WriteLine($"Error: {e.Message}");
                        client = null;
                        continue;
                    }

                    stopwatch.Stop();

                    Console.WriteLine($"{response.Text}{response.Z} [ {(stopwatch.ElapsedTicks * 1000000.0) / Stopwatch.Frequency}us]");
                }

                client?.Dispose();

                Console.WriteLine("Exiting...");
            }
        }
    }
}
