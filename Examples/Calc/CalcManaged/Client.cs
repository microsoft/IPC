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
                var random = new Random();

                for (IClient<Calc.Managed.Request, Calc.Managed.Response> client = null;
                        !exit.WaitOne(TimeSpan.FromSeconds(1)); )
                {
                    if (client == null)
                    {
                        try
                        {
                            client = clientAccessor.Client;

                            Console.WriteLine($"Connected: {client.InputMemory.Name} -> {client.OutputMemory.Name}");

                            client.Closed += (sender, args) => Console.WriteLine($"Connected: {client.InputMemory.Name} -> {client.OutputMemory.Name}");
                        }
                        catch (IPC.Managed.Exception e)
                        {
                            Console.WriteLine($"Error: {e.Message}");
                            continue;
                        }
                    }

                    var request = new Calc.Managed.Request
                    {
                        X = (float)random.NextDouble(1.0, 99.0),
                        Y = (float)random.NextDouble(1.0, 99.0),
                        Op = (Calc.Managed.Operation)random.Next(0, 3)
                    };

                    Task<Calc.Managed.Response> responseTask;
                    var stopwatch = new Stopwatch();
                    stopwatch.Start();

                    try
                    {
                        responseTask = client.InvokeAsync(request);
                    }
                    catch (IPC.Managed.Exception e)
                    {
                        Console.WriteLine($"Error: {e.Message}");
                        client = null;
                        continue;
                    }

                    try
                    {
                        var response = responseTask.Result;
                        stopwatch.Stop();

                        Console.WriteLine($"{response.Text}{response.Z} [ {(stopwatch.ElapsedTicks*1000000.0)/Stopwatch.Frequency}us]");
                    }
                    catch (IPC.Managed.Exception e)
                    {
                        Console.WriteLine($"Error: {e.Message}");
                    }
                }

                Console.WriteLine("Exiting...");
            }
        }
    }
}
