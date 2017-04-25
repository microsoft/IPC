#pragma once

#include "QueueFwd.h"
#include "FixedQueue.h"
#include "ContainerList.h"
#include <new>

#pragma warning(push)
#include <boost/interprocess/exceptions.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
    namespace LockFree
    {
        /// Provides a multi-reader/multi-writer queue with dynamically growing capacity
        /// where pushing and popping is lock-free and capacity growth is synchronized.
        /// Supports any element type. Can also be allocated directly in shared memory
        /// as along as T has same capability.
        template <typename T, typename Allocator, std::size_t BucketSize>
        class Queue
        {
        public:
            explicit Queue(const Allocator& allocator)
                : m_queues{ allocator }
            {}

            bool IsEmpty() const
            {
                return !m_queues.TryApply([](const auto& queue) { return !queue.IsEmpty(); });
            }

            template <typename U>
            bool Push(U&& value)
            {
                try
                {
                    m_queues.Apply([&](auto& queue) { return queue.Push(std::forward<U>(value)); });    // Push won't move out the value on failure.
                    return true;
                }
                catch (const boost::interprocess::bad_alloc& /*e*/)
                {
                    return false;
                }
                catch (const std::bad_alloc& /*e*/)
                {
                    return false;
                }
            }

            boost::optional<T> Pop()
            {
                boost::optional<T> result;

                m_queues.TryApply([&](auto& queue) { return static_cast<bool>(result = queue.Pop()); });

                return result;
            }

            template <typename Function>
            std::size_t ConsumeAll(Function&& func)
            {
                std::size_t count{ 0 };

                m_queues.TryApply([&](auto& queue) { count += queue.ConsumeAll(func); return false; });

                return count;
            }

        private:
            ContainerList<FixedQueue<T, BucketSize>, Allocator> m_queues;
        };

    } // LockFree
    } // detail
} // IPC
