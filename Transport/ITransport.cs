using System;
using System.Threading.Tasks;

namespace IPC.Managed
{
    public delegate Task<Response> Handler<Request, Response>(Request request);

    public delegate Handler<Request, Response> HandlerFactory<Request, Response>(SharedMemory inputMemory, SharedMemory outputMemory);

    [Object]
    public interface ITransport<Request, Response> : IDisposable
    {
        IServerAcceptor<Request, Response> MakeServerAcceptor(string name, HandlerFactory<Request, Response> handlerFactory);

        IClientConnector<Request, Response> MakeClientConnector();

        IClientAccessor<Request, Response> ConnectClient(
            string name, bool async, TimeSpan timeout = default(TimeSpan), IClientConnector<Request, Response> connector = null);

        IServersAccessor<Request, Response> AcceptServers(string name, HandlerFactory<Request, Response> handlerFactory);
    }
}
