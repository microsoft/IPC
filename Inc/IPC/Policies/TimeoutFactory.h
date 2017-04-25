#pragma once

#include "ThreadPool.h"
#include "IPC/detail/Callback.h"
#include <memory>
#include <chrono>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
namespace Policies
{
    class TimeoutFactory
    {
        class Timeout;

        class Scheduler
        {
        public:
            explicit Scheduler(std::shared_ptr<Timeout> timeout);

            ~Scheduler();

            /// Schedules a timeout to fire after predefined amount of time configured in TimeoutFactory.
            /// Previously scheduled timeout is deactivated first.
            void operator()() const;

            /// Schedules a timeout to fire after specified amount of time.
            /// Previously scheduled timeout is deactivated first.
            void operator()(const std::chrono::milliseconds& timeout) const;

            /// Deactivates already scheduled timeout.
            /// Blocks until the running handler (if any) returns.
            void operator()(nullptr_t) const;

        private:
            std::shared_ptr<Timeout> m_timeout;
        };

    public:
        TimeoutFactory(const std::chrono::milliseconds& defaultTimeout = {}, boost::optional<ThreadPool> pool = {});

        Scheduler operator()(detail::Callback<void()> handler) const;

    private:
        std::chrono::milliseconds m_defaultTimeout;
        boost::optional<ThreadPool> m_pool;
    };

} // Policies
} // IPC
