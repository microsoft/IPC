namespace IPC.Managed
{
    public interface IClientAccessor<Request, Response> : IAccessor<IClient<Request, Response>>
    {
        IClient<Request, Response> Client { get; }
    }
}
