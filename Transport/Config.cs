using System;

namespace IPC.Managed
{
    public class Config
    {
        public uint OutputMemorySize { get; set; }

        public TimeSpan ReconnectTimeout { get; set; }

        public TimeSpan DefaultRequestTimeout { get; set; }
    }
}
