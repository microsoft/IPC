using System;

namespace IPC.Managed
{
    [AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface)]
    public class ObjectAttribute : Attribute
    { }
}
