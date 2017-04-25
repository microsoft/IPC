using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;
using NUnit.Framework;

namespace IPC.Managed.UnitTests
{
    using IPC.Containers.Managed;

    [TestFixture(Category = "ManagedTests")]
    public class TransportTests
    {
        private TransportFactory _factory;
        private TransportFactory Factory
        {
            get
            {
                if (_factory == null)
                {
                    _factory = new TransportFactory();
                    _factory.Register(System.Reflection.Assembly.Load("TestManagedTransport"));
                }
                return _factory;
            }
        }

        [Test]
        public async Task AcceptorConnectorTest()
        {
            var address = Guid.NewGuid().ToString();
            bool isClientClosed = false;
            bool isServerClosed = false;

            var config = new Config { DefaultRequestTimeout = System.Threading.Timeout.InfiniteTimeSpan };

            using (var transport = Factory.Make<int, int>(config))
            using (var acceptor = transport.MakeServerAcceptor(address, (inMemory, outMemory) => x => Task.FromResult(x)))
            using (var connector = transport.MakeClientConnector())
            {
                var servers = new List<IServer<int, int>>();
                var newServerEvent = new System.Threading.ManualResetEventSlim(false);

                acceptor.Accepted += (sender, args) =>
                {
                    lock (servers)
                    {
                        servers.Add(args.Component);
                    }

                    newServerEvent.Set();
                };

                using (var client = await connector.ConnectAsync(address))
                {
                    newServerEvent.Wait();

                    IServer<int, int> server;

                    lock (servers)
                    {
                        Assert.AreEqual(1, servers.Count);
                        server = servers.First();
                    }

                    Assert.IsFalse(server.IsClosed);
                    server.Closed += (sender, args) => { isServerClosed = true; };

                    Assert.IsFalse(client.IsClosed);
                    client.Closed += (sender, args) => { isClientClosed = true; };

                    Assert.AreEqual(100, await client.InvokeAsync(100));
                }
            }

            Assert.IsTrue(isClientClosed);
            Assert.IsTrue(isServerClosed);
        }

        [Test]
        public async Task AcceptConnectTest()
        {
            var address = Guid.NewGuid().ToString();

            var config = new Config { DefaultRequestTimeout = System.Threading.Timeout.InfiniteTimeSpan };

            using (var transport = Factory.Make<int, int>(config))
            using (var serversAccessor = transport.AcceptServers(address, (inMemory, outMemory) => x => Task.FromResult(x)))
            using (var clientAccessor = transport.ConnectClient(address, false))
            {
                Assert.AreEqual(100, await clientAccessor.Client.InvokeAsync(100));
            }
        }

        [Test]
        public async Task ContainerTest()
        {
            var address = Guid.NewGuid().ToString();

            var config = new Config { DefaultRequestTimeout = System.Threading.Timeout.InfiniteTimeSpan };

            HandlerFactory<IVector<int>, IVector<int>> handlerFactory =
                (inMemory, outMemory) => request => Task.Run(() =>
                {
                    var response = Factory.Make<IVector<int>>(outMemory);

                    using (request)
                    {
                        response.Reserve(request.Count);

                        foreach (int x in request)
                        {
                            response.Add(x * 2);
                        }
                    }

                    return response;
                });

            using (var transport = Factory.Make<IVector<int>, IVector<int>>(config))
            using (var serversAccessor = transport.AcceptServers(address, handlerFactory))
            using (var clientAccessor = transport.ConnectClient(address, false))
            {
                var client = clientAccessor.Client;

                Task<IVector<int>> responseTask;

                using (var request = Factory.Make<IVector<int>>(client.OutputMemory))
                {
                    request.Resize(100, 123);
                    responseTask = client.InvokeAsync(request);
                }

                using (var response = await responseTask)
                {
                    CollectionAssert.AreEqual(Enumerable.Repeat(123 * 2, 100), response);
                }
            }
        }
    }
}
