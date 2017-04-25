#pragma once

#include "IPC/detail/Callback.h"
#include <memory>
#include <vector>
#include <chrono>
#include <random>


namespace IPC
{
namespace UnitTest
{
namespace Mocks
{
    class TimeoutFactory : public std::shared_ptr<std::vector<std::pair<std::weak_ptr<detail::Callback<void()>>, std::chrono::milliseconds>>>
    {
        class Scheduler
        {
        public:
            Scheduler(detail::Callback<void()> handler, std::shared_ptr<element_type> timeouts, const std::chrono::milliseconds& defaultTimeout);

            void operator()() const;

            void operator()(const std::chrono::milliseconds& timeout) const;

            void operator()(std::nullptr_t);

        private:
            std::shared_ptr<detail::Callback<void()>> m_handler;
            std::shared_ptr<element_type> m_timeouts;
            std::chrono::milliseconds m_defaultTimeout;
        };

    public:
        TimeoutFactory(const std::chrono::milliseconds& defaultTimeout = {});

        std::size_t Process();

        Scheduler operator()(detail::Callback<void()> handler) const;

    private:
        std::shared_ptr<std::random_device> m_rd;
        std::chrono::milliseconds m_defaultTimeout;
    };


} // Mocks
} // UnitTest
} // IPC
