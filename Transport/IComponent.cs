using System;

namespace IPC.Managed
{
    public interface IComponent : IDisposable
    {
        SharedMemory InputMemory { get; }

        SharedMemory OutputMemory { get; }

        bool IsClosed { get; }

        void Close();

        event EventHandler Closed;
    }
}
