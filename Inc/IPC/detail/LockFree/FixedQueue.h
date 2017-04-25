#pragma once

#pragma warning(push)
#include <boost/lockfree/queue.hpp>
#include <boost/optional.hpp>
#pragma warning(pop)

#include <array>


namespace IPC
{
    namespace detail
    {
    namespace LockFree
    {
        /// Provides a multi-reader/multi-writer queue with a compile-time fixed capacity
        /// where pushing and popping is lock-free. Supports any element type. Can also
        /// be allocated directly in shared memory as along as T has same capability.
        template <typename T, std::size_t Capacity, typename Enable = void>
        class FixedQueue;


        /// Specialization for complex (non-trivially copyable) types.
        template <typename T, std::size_t Capacity, typename Enable>
        class FixedQueue
        {
        public:
            FixedQueue()
            {
                for (std::size_t i = 0; i < Capacity; ++i)
                {
                    m_pool.push(i);
                }
            }

            FixedQueue(const FixedQueue& other) = delete;
            FixedQueue& operator=(const FixedQueue& other) = delete;

            ~FixedQueue()
            {
                m_queue.consume_all(
                    [this](std::size_t index)
                    {
                        auto obj = reinterpret_cast<T*>(&m_storage[index]);
                        obj->~T();
                        (void)obj;
                    });
            }

            bool IsEmpty() const
            {
                return m_queue.empty();
            }

            template <typename U>
            bool Push(U&& value)
            {
                std::size_t index;
                if (m_pool.pop(index))
                {
                    try
                    {
                        new (&m_storage[index]) T(std::forward<U>(value));
                    }
                    catch (...)
                    {
                        auto result = m_pool.push(index);
                        assert(result);
                        (void)result;
                        throw;
                    }

                    auto result = m_queue.push(index);
                    assert(result);

                    return result;
                }

                return false;
            }

            boost::optional<T> Pop()
            {
                boost::optional<T> value;

                std::size_t index;
                if (m_queue.pop(index))
                {
                    auto obj = reinterpret_cast<T*>(&m_storage[index]);

                    auto restoreIndex = [&]
                    {
                        auto result = m_pool.push(index);
                        assert(result);
                        (void)result;
                    };

                    try
                    {
                        value = std::move(*obj);
                        obj->~T();
                    }
                    catch (...)
                    {
                        restoreIndex();
                        throw;
                    }

                    restoreIndex();
                }

                return value;
            }

            template <typename Function>
            std::size_t ConsumeAll(Function&& func)
            {
                return m_queue.consume_all(
                    [this, &func](std::size_t index)
                    {
                        auto obj = reinterpret_cast<T*>(&m_storage[index]);

                        auto restoreIndex = [&]
                        {
                            auto result = m_pool.push(index);
                            assert(result);
                            (void)result;
                        };

                        try
                        {
                            func(std::move(*obj));
                            obj->~T();
                        }
                        catch (...)
                        {
                            restoreIndex();
                            throw;
                        }

                        restoreIndex();
                    });
            }

        private:
            using Queue = boost::lockfree::queue<std::size_t, boost::lockfree::capacity<Capacity>>;

            Queue m_pool;
            Queue m_queue;
            std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, Capacity> m_storage;
        };


        /// Specialization for trivially copyable types.
        template <typename T, std::size_t Capacity>
        class FixedQueue<T, Capacity, std::enable_if_t<boost::has_trivial_destructor<T>::value && boost::has_trivial_assign<T>::value>>
        {
        public:
            bool IsEmpty() const
            {
                return m_queue.empty();
            }

            bool Push(const T& value)
            {
                return m_queue.push(value);
            }

            boost::optional<T> Pop()
            {
                T value;
                return m_queue.pop(value) ? boost::optional<T>{ std::move(value) } : boost::none;
            }

            template <typename Function>
            std::size_t ConsumeAll(Function&& func)
            {
                return m_queue.consume_all([this, &func](T& value) { func(std::move(value)); });
            }

        private:
            boost::lockfree::queue<T, boost::lockfree::capacity<Capacity>> m_queue;
        };

    } // LockFree
    } // detail
} // IPC
