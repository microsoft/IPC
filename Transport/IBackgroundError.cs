using System;

namespace IPC.Managed
{
    public interface IBackgroundError : IDisposable
    {
        event EventHandler<ErrorEventArgs> Error;
    }
}
