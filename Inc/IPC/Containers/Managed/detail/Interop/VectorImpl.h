#pragma once

#include "Vector.h"
#include "IPC/Containers/Vector.h"


namespace IPC
{
namespace Containers
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename T>
        struct Vector<T>::BaseIterator : IPC::Containers::Vector<T>::element_type::iterator
        {
            using Base = typename IPC::Containers::Vector<T>::element_type::iterator;

            using Base::Base;

            BaseIterator(const Base& other)
                : Base{ other }
            {}

            BaseIterator(Base&& other)
                : Base{ std::move(other) }
            {}
        };


        template <typename T>
        Vector<T>::Iterator::Iterator() = default;

        template <typename T>
        Vector<T>::Iterator::Iterator(const BaseIterator& it)
            : shared_ptr{ std::make_shared<BaseIterator>(it) }
        {}

        template <typename T>
        Vector<T>::Iterator::Iterator(const Iterator& other) = default;

        template <typename T>
        Vector<T>::Iterator::Iterator(Iterator&& other) = default;

        template <typename T>
        Vector<T>::Iterator::~Iterator() = default;

        template <typename T>
        bool Vector<T>::Iterator::MoveNext(const Vector& container)
        {
            if (!this->operator bool())
            {
                shared_ptr::operator=(std::make_shared<BaseIterator>(container.m_impl->begin()));
            }
            else if (*this->get() != container.m_impl->end())
            {
                ++*this->get();
            }

            return *this->get() != container.m_impl->end();
        }

        template <typename T>
        T& Vector<T>::Iterator::GetCurrent(const Vector& container)
        {
            if (!this->operator bool() || *this->get() == container.m_impl->end())
            {
                throw std::runtime_error{ "Invalid operation." };
            }

            return **this->get();
        }

        template <typename T>
        void Vector<T>::Iterator::Reset()
        {
            this->reset();
        }

        template <typename T>
        auto Vector<T>::Iterator::operator++() -> Iterator&
        {
            ++*this->get();
            return *this;
        }

        template <typename T>
        T& Vector<T>::Iterator::operator*()
        {
            return **this->get();
        }


        template <typename T>
        Vector<T>::Vector(IPC::Containers::Vector<T>& impl)
            : m_impl{ impl }
        {}

        template <typename T>
        void Vector<T>::Reserve(std::size_t size)
        {
            m_impl->reserve(size);
        }

        template <typename T>
        void Vector<T>::Resize(std::size_t size, const T& value)
        {
            m_impl->resize(size, value);
        }

        template <typename T>
        std::size_t Vector<T>::GetCapacity() const
        {
            return m_impl->capacity();
        }

        template <typename T>
        std::size_t Vector<T>::GetCount() const
        {
            return m_impl->size();
        }

        template <typename T>
        auto Vector<T>::begin() -> Iterator
        {
            return m_impl->begin();
        }

        template <typename T>
        auto Vector<T>::end() -> Iterator
        {
            return m_impl->end();
        }

        template <typename T>
        void Vector<T>::Add(const T& value)
        {
            m_impl->push_back(value);
        }

        template <typename T>
        void Vector<T>::Insert(std::size_t index, const T& value)
        {
            if (index > GetCount())
            {
                throw std::out_of_range{ "index" };
            }

            m_impl->insert(m_impl->begin() + index, value);
        }

        template <typename T>
        bool Vector<T>::Contains(const T& value) const
        {
            return std::find(m_impl->begin(), m_impl->end(), value) != m_impl->end();
        }

        template <typename T>
        int Vector<T>::IndexOf(const T& value) const
        {
            auto it = std::find(m_impl->begin(), m_impl->end(), value);

            return it != m_impl->end() ? static_cast<int>(std::distance(m_impl->begin(), it)) : -1;
        }

        template <typename T>
        bool Vector<T>::Remove(const T& value)
        {
            auto it = std::find(m_impl->begin(), m_impl->end(), value);

            if (it == m_impl->end())
            {
                return false;
            }

            m_impl->erase(it);

            return true;
        }

        template <typename T>
        void Vector<T>::RemoveAt(std::size_t index)
        {
            if (index >= GetCount())
            {
                throw std::out_of_range{ "index" };
            }

            m_impl->erase(m_impl->begin() + index);
        }

        template <typename T>
        const T& Vector<T>::GetAt(std::size_t index) const
        {
            return m_impl->at(index);
        }

        template <typename T>
        void Vector<T>::SetAt(std::size_t index, const T& value)
        {
            m_impl->at(index) = value;
        }

        template <typename T>
        void Vector<T>::Clear()
        {
            return m_impl->clear();
        }


        template <typename T>
        IPC::Containers::Vector<T>* Vector<T>::ExternalConstructor::New(const IPC::Containers::Vector<T>& other)
        {
            return new IPC::Containers::Vector<T>{ other };
        }

        template <typename T>
        IPC::Containers::Vector<T>* Vector<T>::ExternalConstructor::New(IPC::Managed::detail::Interop::SharedMemory memory)
        {
            return new IPC::Containers::Vector<T>{ std::move(memory) };
        }

        template <typename T>
        void Vector<T>::ExternalConstructor::Delete(IPC::Containers::Vector<T>* obj)
        {
            delete obj;
        }

    } // Interop
    } // detail

} // Managed
} // Containers
} // IPC
