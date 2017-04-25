using System;

namespace IPC.Managed
{
    public interface IAccessor<T> : IBackgroundError
        where T : class, IComponent
    {
        event EventHandler<ComponentEventArgs<T>> Connected;

        event EventHandler<ComponentEventArgs<T>> Disconnected;
    }
}
