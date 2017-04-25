using System;
using System.Threading.Tasks;

namespace IPC.Managed
{
    public interface IClientConnector<Request, Response> : IDisposable
    {
        Task<IClient<Request, Response>> ConnectAsync(string acceptorName, TimeSpan timeout = default(TimeSpan));
    }
}
