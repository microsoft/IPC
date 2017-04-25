using System;
using System.Threading.Tasks;

namespace IPC.Managed
{
    public interface IClient<Request, Response> : IComponent
    {
        Task<Response> InvokeAsync(Request request, TimeSpan timeout = default(TimeSpan));
    }
}
