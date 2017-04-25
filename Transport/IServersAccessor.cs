using System.Collections.Generic;

namespace IPC.Managed
{
    public interface IServersAccessor<Request, Response> : IAccessor<IServer<Request, Response>>
    {
        IReadOnlyCollection<IServer<Request, Response>> Servers { get; }
    }
}
