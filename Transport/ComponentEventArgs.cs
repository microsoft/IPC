using System;

namespace IPC.Managed
{
    public class ComponentEventArgs<T> : EventArgs
        where T : IComponent
    {
        private readonly T _component;

        public ComponentEventArgs(T component)
        {
            _component = component;
        }

        public T Component
        {
            get { return _component; }
        }
    }
}
