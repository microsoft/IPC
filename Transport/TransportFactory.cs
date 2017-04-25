namespace IPC.Managed
{
    public class TransportFactory : ObjectFactory
    {
        public TransportFactory(bool registerAll = false)
            : base(registerAll)
        { }

        public ITransport<Request, Response> Make<Request, Response>(Config config = null)
        {
            return MakeObject<ITransport<Request, Response>>(config ?? new Config());
        }
    }
}
