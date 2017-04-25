#include "stdafx.h"
#include "TimeoutFactoryMock.h"


namespace IPC
{
namespace UnitTest
{
namespace Mocks
{
    TimeoutFactory::Scheduler::Scheduler(detail::Callback<void()> handler, std::shared_ptr<element_type> timeouts, const std::chrono::milliseconds& defaultTimeout)
        : m_handler{ std::make_shared<detail::Callback<void()>>(std::move(handler)) },
          m_timeouts{ std::move(timeouts) },
          m_defaultTimeout{ defaultTimeout }
    {}

    void TimeoutFactory::Scheduler::operator()() const
    {
        operator()(m_defaultTimeout);
    }

    void TimeoutFactory::Scheduler::operator()(const std::chrono::milliseconds& timeout) const
    {
        m_timeouts->emplace_back(m_handler, timeout);
    }

    void TimeoutFactory::Scheduler::operator()(std::nullptr_t)
    {
        m_handler = {};
    }


    TimeoutFactory::TimeoutFactory(const std::chrono::milliseconds& defaultTimeout)
        : shared_ptr{ std::make_shared<element_type>() },
          m_rd{ std::make_shared<std::random_device>() },
          m_defaultTimeout{ defaultTimeout }
    {}

    std::size_t TimeoutFactory::Process()
    {
        std::size_t count{ 0 };
        element_type items;

        std::swap(*get(), items);

        std::shuffle(items.begin(), items.end(), std::mt19937{ (*m_rd)() });

        for (auto& item : items)
        {
            if (auto handler = item.first.lock())
            {
                ++count;
                (*handler)();
            }
        }

        return count;
    }

    auto TimeoutFactory::operator()(detail::Callback<void()> handler) const -> Scheduler
    {
        return{ std::move(handler), *this, m_defaultTimeout };
    }


} // Mocks
} // UnitTest
} // IPC
