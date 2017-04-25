#pragma once

#include "IPC/Managed/detail/Interop/Callback.h"
#include <functional>
#include <memory>
#include <chrono>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
    namespace Policies
    {
        class TimeoutFactory
        {
            using TimeoutHandle = std::shared_ptr<void>;
            using TimeoutChangeFunc = std::function<void(const std::chrono::milliseconds&)>;
            using TimeoutFactoryFunc = std::function<std::pair<TimeoutHandle, TimeoutChangeFunc>(Callback<void()>&&)>;

            class Scheduler
            {
            public:
                Scheduler(Callback<void()> handler, const TimeoutFactoryFunc& timeoutFatory, const std::chrono::milliseconds& defaultTimeout);

                /// Schedules a timeout to fire after predefined amount of time configured in TimeoutFactory.
                /// Previously scheduled timeout is deactivated first.
                void operator()();

                /// Schedules a timeout to fire after specified amount of time.
                /// Previously scheduled timeout is deactivated first.
                void operator()(const std::chrono::milliseconds& timeout);

                /// Deactivates already scheduled timeout.
                /// Blocks until the running handler (if any) returns.
                void operator()(std::nullptr_t);

            private:
                std::shared_ptr<Callback<void()>> m_handler;
                TimeoutFactoryFunc m_timeoutFatory;
                TimeoutHandle m_timeoutHandle;
                std::chrono::milliseconds m_defaultTimeout;
            };

        public:
            TimeoutFactory(const std::chrono::milliseconds& defaultTimeout, const TimeoutFactoryFunc& timeoutFatory);

            Scheduler operator()(Callback<void()> handler) const;

        private:
            std::chrono::milliseconds m_defaultTimeout;
            TimeoutFactoryFunc m_timeoutFatory;
        };

    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
