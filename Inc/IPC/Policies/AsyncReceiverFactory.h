#pragma once

#include "ThreadPool.h"
#include "IPC/detail/Callback.h"
#include <memory>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
namespace Policies
{
    class AsyncReceiverFactory
    {
    public:
        AsyncReceiverFactory() = default;

        explicit AsyncReceiverFactory(boost::optional<ThreadPool> pool);

        template <typename Queue, typename Handler>
        auto operator()(Queue& queue, Handler&& handler) const
        {
            return Scheduler{
                [&queue, handler = std::forward<Handler>(handler)]
                {
                    if (auto&& value = queue.Pop())
                    {
                        handler(std::move(*value));
                    }
                },
                m_pool };
        }

    private:
        class Scheduler
        {
        public:
            Scheduler(detail::Callback<void()> handler, boost::optional<ThreadPool> pool);

            Scheduler(Scheduler&& other);

            ~Scheduler();

            std::size_t operator()() const;

        private:
            class Impl;

            std::shared_ptr<Impl> m_impl;
        };


        boost::optional<ThreadPool> m_pool;
    };

} // Policies
} // IPC
