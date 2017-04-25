#pragma once

#include "Interop/Vector.h"
#include "IPC/Managed/detail/NativeObject.h"
#include "IPC/Managed/detail/Cast.h"


namespace IPC
{
namespace Containers
{
namespace Managed
{
    namespace detail
    {
        using IPC::Managed::detail::CastType;
        using IPC::Managed::detail::Cast;

        template <typename T>
        [IPC::Managed::Object]
        ref class Vector : IVector<T>
        {
        internal:
            Vector(const IPC::Containers::Vector<CastType<T>>& other)
                : m_vector{ other }
            {}

        public:
            Vector(IPC::Managed::SharedMemory^ memory)
                : m_vector{ memory->Impl }
            {}

            virtual System::Collections::Generic::IEnumerator<T>^ GetEnumerator()
            {
                return gcnew Enumerator{ this };
            }

            virtual System::Collections::IEnumerator^ GetEnumeratorNonGeneric() = System::Collections::IEnumerable::GetEnumerator
            {
                return GetEnumerator();
            }

            virtual void Resize(System::Int32 size, T value)
            {
                Impl.Resize(size, Cast(value));
            }

            property System::Int32 Capacity
            {
                virtual System::Int32 get() { return static_cast<System::Int32>(Impl.GetCapacity()); }
            }

            virtual void Reserve(System::Int32 size)
            {
                Impl.Reserve(size);
            }

            property System::Int32 Count
            {
                virtual System::Int32 get() { return static_cast<System::Int32>(Impl.GetCount()); }
            }

            property System::Boolean IsReadOnly
            {
                virtual System::Boolean get() { return false; }
            }

            virtual void Add(T item)
            {
                Impl.Add(Cast(item));
            }

            virtual void Clear()
            {
                Impl.Clear();
            }

            virtual System::Boolean Contains(T item)
            {
                return Impl.Contains(Cast(item));
            }

            virtual System::Boolean Remove(T item)
            {
                return Impl.Remove(Cast(item));
            }

            virtual void CopyTo(cli::array<T>^ arr, System::Int32 index)
            {
                if (!arr)
                {
                    throw gcnew System::NullReferenceException{};
                }

                if (index < 0 || index + Impl.GetCount() >= arr->Length)
                {
                    throw gcnew System::ArgumentOutOfRangeException{};
                }

                for (auto&& item : Impl)
                {
                    arr[index++] = Cast(item);
                }
            }

            property T default[System::Int32]
            {
                virtual T get(System::Int32 index)
                {
                    try
                    {
                        return Cast(Impl.GetAt(index));
                    }
                    catch (const std::out_of_range&)
                    {
                        throw gcnew System::ArgumentOutOfRangeException{};
                    }
                }

                virtual void set(System::Int32 index, T value)
                {
                    try
                    {
                        Impl.SetAt(index, Cast(value));
                    }
                    catch (const std::out_of_range&)
                    {
                        throw gcnew System::ArgumentOutOfRangeException{};
                    }
                }
            }

            virtual System::Int32 IndexOf(T item)
            {
                return Impl.IndexOf(Cast(item));
            }

            virtual void RemoveAt(System::Int32 index)
            {
                try
                {
                    Impl.RemoveAt(index);
                }
                catch (const std::out_of_range&)
                {
                    throw gcnew System::ArgumentOutOfRangeException{};
                }
            }

            virtual void Insert(System::Int32 index, T item)
            {
                try
                {
                    Impl.Insert(index, Cast(item));
                }
                catch (const std::out_of_range&)
                {
                    throw gcnew System::ArgumentOutOfRangeException{};
                }
            }

        internal:
            IPC::Managed::detail::NativeObject<IPC::Containers::Vector<CastType<T>>> m_vector;

        private:
            ref class Enumerator : System::Collections::Generic::IEnumerator<T>
            {
            public:
                Enumerator(Vector<T>^ vector)
                    : m_vector{ vector }
                {}

                virtual System::Boolean MoveNext()
                {
                    return m_iterator->MoveNext(m_vector->Impl);
                }

                property T Current
                {
                    virtual T get()
                    {
                        try
                        {
                            return Cast(m_iterator->GetCurrent(m_vector->Impl));
                        }
                        catch (const std::runtime_error&)
                        {
                            throw gcnew System::InvalidOperationException{};
                        }
                    }
                }

                property System::Object^ CurrentNonGeneric
                {
                    virtual System::Object^ get() = System::Collections::IEnumerator::Current::get
                    {
                        return Current;
                    }
                }

                virtual void Reset()
                {
                    m_iterator->Reset();
                }

            private:
                IPC::Managed::detail::NativeObject<typename Interop::Vector<CastType<T>>::Iterator> m_iterator;
                Vector<T>^ m_vector;
            };


            property Interop::Vector<CastType<T>> Impl
            {
                Interop::Vector<CastType<T>> get()
                {
                    return *m_vector;
                }
            }
        };

    } // detail

} // Managed
} // Containers
} // IPC



namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T>
        struct Convert<IPC::Containers::Vector<T>>
        {
            using type = IPC::Containers::Managed::IVector<CastType<T>>^;

            static const IPC::Containers::Vector<T>& From(type% from)
            {
                return *safe_cast<IPC::Containers::Managed::detail::Vector<CastType<T>>^>(from)->m_vector;
            }
        };

        template <typename T>
        struct Convert<IPC::Containers::Managed::IVector<T>^>
        {
            using type = IPC::Containers::Vector<CastType<T>>;

            static IPC::Containers::Managed::IVector<T>^ From(const type& from)
            {
                return gcnew IPC::Containers::Managed::detail::Vector<T>{ from };
            }
        };

    } // detail

} // Managed
} // IPC
