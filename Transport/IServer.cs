namespace IPC.Managed
{
    public interface IServer<Request, Response> : IComponent, IBackgroundError
    { }
}
