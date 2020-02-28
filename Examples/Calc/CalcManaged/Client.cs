using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using IPC.Managed;

namespace Calc.Managed
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
            Console.WriteLine($"Connecting to {address}");

            using (var exit = new ManualResetEvent(false))
            using (var transport = factory.Make<Request, Response>())
            using (var clientAccessor = transport.ConnectClient(address, true))
            {
                Console.CancelKeyPress += (sender, args) => { args.Cancel = true; exit.Set(); };

                clientAccessor.Error += (sender, args) => Console.WriteLine($"IPC: {args.Exception.Message}");

                var random = new Random();
                IClient<Request, Response> client = null;

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
                            Console.WriteLine($"Failed to access the client: {e.Message}");
                            continue;
                        }

                        var info = $"{client.InputMemory.Name} -> {client.OutputMemory.Name}";

                        Console.WriteLine($"Connected: {info}");

                        client.Closed += (sender, args) => Console.WriteLine($"Disconnected: {info}");
                    }

                    var request = new Request
                    {
                        X = (float)random.NextDouble(1.0, 99.0),
                        Y = (float)random.NextDouble(1.0, 99.0),
                        Op = (Operation)random.Next(0, 4)
                    };

                    Response response;

                    var stopwatch = new Stopwatch();
                    stopwatch.Start();

                    try
                    {
                        response = client.InvokeAsync(request).Result;
                    }
                    catch (System.Exception e)
                    {
                        Console.WriteLine($"Failed to send request: {e.Message}");
                        client = null;
                        continue;
                    }

                    stopwatch.Stop();

                    Console.WriteLine($"{response.Text}{response.Z} [{(stopwatch.ElapsedTicks * 1000000.0) / Stopwatch.Frequency}us]");
                }

                Console.WriteLine("Exiting...");

                client?.Dispose();
            }
        }
    }
}
