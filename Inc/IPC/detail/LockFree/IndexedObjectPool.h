#pragma once

#include "Queue.h"
#include "ContainerList.h"
#include <memory>
#include <array>
#include <atomic>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
    namespace LockFree
    {
        /// Provides an object pool of any type with dynamically growing capacity
        /// where each object is referred by index. Object retrieval and return are
        /// lock-free. Capacity growth is synchronized. Can be allocated directly in
        /// shared memory as along as T has same capability.
        template <typename T, typename Allocator, std::uint32_t BucketSize = 64>
        class IndexedObjectPool
        {
        public:
            using Index = std::uint32_t;

            IndexedObjectPool()
                : IndexedObjectPool{ {} }
            {}

            explicit IndexedObjectPool(const Allocator& allocator)
                : m_buckets{ allocator },
                  m_pool{ allocator }
            {}

            /// Retrieves or constructs a new object with index as first argument
            /// followed by provided arguments.
            /// \returns A pair of object reference and index.
            template <typename... Args>
            std::pair<T&, Index> Take(Args&&... args)
            {
                if (auto index = m_pool.Pop())
                {
                    auto item = Get(*index);
                    assert(item);

                    item->MarkUsed();

                    return{ **item, *index };
                }

                return Construct(std::forward<Args>(args)...);
            }

            /// Returns the object with given index back to the pool.
            bool Return(Index index)
            {
                return Return(index, [](T&) {});
            }

            /// Returns the object with given index back to the pool and
            /// invokes the function before it becomes available for retrieval.
            template <typename Function>
            bool Return(Index index, Function&& func)
            {
                if (auto item = Get(index))
                {
                    if (item->MarkFree())
                    {
                        try
                        {
                            std::forward<Function>(func)(**item);
                        }
                        catch (...)
                        {
                            m_pool.Push(index);
                            throw;
                        }

                        m_pool.Push(index);
                        return true;
                    }
                }

                return false;
            }

            /// Returns all objects back to pool.
            void ReturnAll()
            {
                ReturnAll([](T&) {});
            }

            /// Returns all objects back to pool and invokes the function
            /// before each of them becomes available for retrieval.
            template <typename Function>
            void ReturnAll(Function&& func)
            {
                Apply(
                    [&](auto& item)
                    {
                        assert(item);
                        if (item.MarkFree())
                        {
                            func(*item);
                        }
                    });
            }

        private:
            class Bucket
            {
            public:
                class Item : public boost::optional<T>
                {
                public:
                    void MarkUsed()
                    {
                        m_free.clear();
                    }

                    bool MarkFree()
                    {
                        return !m_free.test_and_set();
                    }

                private:
                    std::atomic_flag m_free{ ATOMIC_FLAG_INIT };
                };


                template <typename... Args>
                std::pair<T*, Index> Construct(Index offset, Args&&... args)
                {
                    auto index = m_size++;
                    if (index < BucketSize)
                    {
                        auto& item = m_items[index];
                        index += offset;

                        item.emplace(index, std::forward<Args>(args)...);   // Intentionally waste the slot if .ctor throws.

                        return{ &*item, index };
                    }

                    --m_size;
                    return{};
                }

                Item* operator[](Index index)
                {
                    return index < m_size.load(std::memory_order_relaxed) ? &m_items[index] : nullptr;
                }

                template <typename Function>
                void Apply(Function&& func)
                {
                    for (auto it = m_items.begin(), end = it;
                            it != (end = std::next(m_items.begin(), m_size.load(std::memory_order_relaxed))); )
                    {
                        for (; it != end; ++it)
                        {
                            if (auto& item = *it)   // Item might be optional::empty due to exception in .ctor.
                            {
                                func(item);
                            }
                        }
                    }
                }

            private:
                std::array<Item, BucketSize> m_items{};
                std::atomic_uint32_t m_size{ 0 };
            };


            typename Bucket::Item* Get(Index index)
            {
                typename Bucket::Item* item{ nullptr };

                auto search = [&item, i = Index{}, itemIndex = index % BucketSize, bucketIndex = index / BucketSize](auto& bucket) mutable
                {
                    if (i == bucketIndex)
                    {
                        item = bucket[itemIndex];
                        return true;
                    }

                    ++i;
                    return false;
                };

                return m_buckets.TryApply(search) ? item : nullptr;
            }

            template <typename... Args>
            __declspec(noinline) std::pair<T&, Index> Construct(Args&&... args)
            {
                std::pair<T*, Index> result;

                m_buckets.Apply(
                    [&, i = Index{}](auto& bucket) mutable
                    {
                        result = bucket.Construct(i++ * BucketSize, std::forward<decltype(args)>(args)...);  // TODO: Use Args when VC14 bugs are fixed.
                        return result.first != nullptr;
                    });

                assert(result.first);
                return{ *result.first, result.second };
            }

            template <typename Function>
            void Apply(Function&& func)
            {
                m_buckets.TryApply([&](auto& bucket) { bucket.Apply(func); return false; });
            }


            ContainerList<Bucket, Allocator> m_buckets;
            Queue<Index, Allocator, BucketSize> m_pool;
        };

    } // LockFree
    } // detail
} // IPC
