using System;
using System.Collections.Generic;
using System.Reflection;
using System.Linq;

namespace IPC.Managed
{
    public class ObjectFactory
    {
        private readonly Dictionary<Type, Type> _mapping = new Dictionary<Type, Type>();

        public ObjectFactory(bool registerAll = false)
        {
            if (registerAll)
            {
                Register();
            }
        }

        public int Register()
        {
            return Register(AppDomain.CurrentDomain.GetAssemblies(), Register);
        }

        public int Register(Assembly assembly)
        {
            return Register(assembly.GetTypes(), Register);
        }

        public int Register(Type type)
        {
            return Register(type.GetInterfaces(), inter => { _mapping.Add(inter, type); return 1; });
        }

        private int Register<T>(IEnumerable<T> items, Func<T, int> register)
            where T : ICustomAttributeProvider
        {
            return items
                .Where(item => item.IsDefined(typeof(ObjectAttribute), false))
                .Aggregate(0, (count, item) => count + register(item));
        }

        public int Count
        {
            get { return _mapping.Count; }
        }

        public T Make<T>(SharedMemory memory)
        {
            return MakeObject<T>(memory);
        }

        protected T MakeObject<T>(params object[] args)
        {
            return (T)Activator.CreateInstance(_mapping[typeof(T)], args);
        } 
    }
}
