#pragma once
#pragma managed(push, off)

#include "IPC/Managed/detail/Interop/SharedMemory.h"
#include "IPC/Managed/detail/Interop/ExternalConstructor.h"
#include <memory>


namespace IPC
{
namespace Containers
{
    template <typename T>
    class Vector;

namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename T>
        class Vector
        {
            struct BaseIterator;

        public:
            struct ExternalConstructor
            {
                static IPC::Containers::Vector<T>* New(const IPC::Containers::Vector<T>& other);

                static IPC::Containers::Vector<T>* New(IPC::Managed::detail::Interop::SharedMemory memory);

                static void Delete(IPC::Containers::Vector<T>* obj);
            };


            struct Iterator : public std::shared_ptr<BaseIterator>
            {
                Iterator();

                Iterator(const BaseIterator& it);

                Iterator(const Iterator& other);
                Iterator(Iterator&& other);

                ~Iterator();

                bool MoveNext(const Vector& container);

                T& GetCurrent(const Vector& container);

                void Reset();

                Iterator& operator++();

                T& operator*();
            };


            Vector(IPC::Containers::Vector<T>& impl);

            void Reserve(std::size_t size);

            void Resize(std::size_t size, const T& value);

            std::size_t GetCapacity() const;

            std::size_t GetCount() const;

            Iterator begin();

            Iterator end();

            void Add(const T& value);

            void Insert(std::size_t index, const T& value);

            bool Contains(const T& value) const;

            int IndexOf(const T& value) const;

            bool Remove(const T& value);

            void RemoveAt(std::size_t index);

            const T& GetAt(std::size_t index) const;

            void SetAt(std::size_t index, const T& value);

            void Clear();

        private:
            IPC::Containers::Vector<T>& m_impl;
        };

    } // Interop
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
    namespace Interop
    {
        template <typename T>
        struct ExternalConstructor<IPC::Containers::Vector<T>>
            : std::true_type,
              IPC::Containers::Managed::detail::Interop::Vector<T>::ExternalConstructor
        {};

    } // Interop
    } // detail

} // Managed
} // IPC

#pragma managed(pop)
