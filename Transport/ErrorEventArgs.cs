using System;

namespace IPC.Managed
{
    public class ErrorEventArgs : EventArgs
    {
        private readonly System.Exception _error;

        public ErrorEventArgs(System.Exception e)
        {
            _error = e;
        }

        public System.Exception Exception
        {
            get { return _error; }
        }
    }
}
