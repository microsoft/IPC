using System;
using System.Linq;
using NUnit.Framework;

namespace IPC.Containers.Managed.UnitTests
{
    using IPC.Managed;

    [TestFixture(Category = "ManagedTests")]
    public class ContainersTests
    {
        private ObjectFactory _factory;
        private ObjectFactory Factory
        {
            get
            {
                if (_factory == null)
                {
                    _factory = new ObjectFactory();
                    _factory.Register(System.Reflection.Assembly.Load("TestManagedTransport"));
                }
                return _factory;
            }
        }

        [Test]
        public void VectorTest()
        {
            var name = Guid.NewGuid().ToString();

            using (var memory = SharedMemory.Create(name, 1024 * 1024))
            using (var v = Factory.Make<IVector<int>>(memory))
            {
                const int N = 100;

                for (int i = 0; i < N; ++i)
                {
                    v.Add(i);
                }

                v.Resize(2*N, 1);

                Assert.AreEqual(2*N, v.Count);
                Assert.AreEqual(N + (N-1) * N / 2, v.Sum());
            }
        }

        [Test]
        public void NestedVectorTest()
        {
            var name = Guid.NewGuid().ToString();

            using (var memory = SharedMemory.Create(name, 1024 * 1024))
            using (var vv = Factory.Make<IVector<IVector<int>>>(memory))
            {
                const int N = 5;
                const int x = 123;

                for (int i = 0; i < N; ++i)
                {
                    using (var v = Factory.Make<IVector<int>>(memory))
                    {
                        v.Resize(N, x);
                        Assert.AreEqual(N, v.Count);

                        vv.Add(v);
                    }
                }

                Assert.AreEqual(N, vv.Count);
                Assert.AreEqual(N * N * x, vv.Sum((v) => { using (v) return v.Sum(); }));
            }
        }
    }
}
