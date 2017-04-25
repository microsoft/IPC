#pragma once

#include "IPC/detail/SpinLock.h"
#include <memory>
#include <atomic>
#include <mutex>


namespace IPC
{
    namespace detail
    {
    namespace LockFree
    {
        /// Provides a lock-free access to a list of containers with dynamically growing
        /// capacity. Capacity growth is synchronized. Can be allocated directly in
        /// shared memory as along as container type has same capability.
        template <typename Container, typename AllocatorT>
        class ContainerList
        {
        public:
            /// Constructs a list with a single container node.
            template <typename... Args>
            explicit ContainerList(const AllocatorT& allocator, Args&&... args)
                : m_allocator{ allocator },
                  m_head{ std::forward<Args>(args)... }
            {}

            /// Applies the provided function to each constant container node in a list.
            /// Iteration stops when function returns true or end of list is reached.
            template <typename Function>
            bool TryApply(Function&& func) const
            {
                return m_head.TryApply(std::forward<Function>(func)) == nullptr;
            }

            /// Applies the provided function to each container node in a list.
            /// Iteration stops when function returns true or end of list is reached.
            template <typename Function>
            bool TryApply(Function&& func)
            {
                return m_head.TryApply(std::forward<Function>(func)) == nullptr;
            }

            /// Applies the provided function to each container node in a list.
            /// Iteration continues until the function returns true. New nodes
            /// are constructed with the provided arguments when end of list is reached.
            template <typename Function, typename... Args>
            void Apply(Function&& func, Args&&... args)
            {
                m_head.Apply(std::forward<Function>(func), m_allocator, m_lock, std::forward<Args>(args)...);
            }

        private:
            class Node
            {
            public:
                using Allocator = typename std::allocator_traits<AllocatorT>::template rebind_alloc<Node>;

                template <typename... Args>
                Node(Args&&... args)
                    : m_container{ std::forward<Args>(args)... }
                {}

                ~Node()
                {
                    auto node = std::move(m_next);  // Manual iterative destruction to avoid stack overflow.
                    while (node)
                    {
                        auto next = std::move(node->m_next);
                        node = std::move(next);
                    }
                }

                template <typename Function>
                const Node* TryApply(Function&& func) const
                {
                    return TryApply(std::forward<Function>(func), this);
                }

                template <typename Function>
                Node* TryApply(Function&& func)
                {
                    return TryApply(std::forward<Function>(func), this);
                }

                template <typename Function, typename... Args>
                void Apply(Function&& func, Allocator& allocator, SpinLock& lock, Args&&... args)
                {
                    if (auto node = TryApply(func))
                    {
                        for (auto newNode = Allocate(allocator, args...);
                                !node->UpdateTail(func, lock, std::move(newNode))
                                && ((node = node->TryApply(func)) != nullptr); )
                        {}
                    }
                }

            private:
                class Deleter
                {
                public:
                    using pointer = typename std::allocator_traits<Allocator>::pointer;

                    Deleter() = default;

                    explicit Deleter(Allocator& allocator)
                        : m_allocator{ &allocator }
                    {}

                    void operator()(const pointer& node) const
                    {
                        assert(node);
                        assert(m_allocator);

                        std::allocator_traits<Allocator>::destroy(*m_allocator, std::addressof(*node));
                        std::allocator_traits<Allocator>::deallocate(*m_allocator, node, 1);
                    }

                private:
                    using AllocatorPtr = typename std::allocator_traits<
                        typename std::allocator_traits<Allocator>::template rebind_alloc<Allocator>>::pointer;

                    AllocatorPtr m_allocator{};
                };


                template <typename Function, typename NodeT>
                static NodeT* TryApply(Function&& func, NodeT* node)
                {
                    assert(node);
                    for (; ; node = std::addressof(*node->m_next))
                    {
                        if (func(node->m_container))
                        {
                            return nullptr;
                        }

                        if (node->m_tail.load(std::memory_order_relaxed))
                        {
                            return node;
                        }
                    }
                }

                template <typename Function>
                __declspec(noinline) bool UpdateTail(Function&& func, SpinLock& lock, std::unique_ptr<Node, Deleter>&& newNode)
                {
                    assert(newNode);
                    std::lock_guard<SpinLock> guard{ lock };

                    if (m_tail.load(std::memory_order_acquire) && func(newNode->m_container))
                    {
                        m_next = std::move(newNode);
                        m_tail.store(false, std::memory_order_release); // Clear the flag after m_next is set.
                        return true;
                    }

                    return false;
                }

                template <typename... Args>
                __declspec(noinline) static auto Allocate(Allocator& allocator, Args&&... args)
                {
                    auto node = std::allocator_traits<Allocator>::allocate(allocator, 1);

                    try
                    {
                        std::allocator_traits<Allocator>::construct(allocator, std::addressof(*node), std::forward<Args>(args)...);
                    }
                    catch (...)
                    {
                        std::allocator_traits<Allocator>::deallocate(allocator, node, 1);
                        throw;
                    }

                    return std::unique_ptr<Node, Deleter>{ std::move(node), Deleter{ allocator } };
                }


                Container m_container;
                std::atomic_bool m_tail{ true };
                std::unique_ptr<Node, Deleter> m_next;
            };


            typename Node::Allocator m_allocator;   // Allocator must be the first one.
            Node m_head;
            SpinLock m_lock;
        };

    } // LockFree
    } // detail
} // IPC
