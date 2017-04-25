#include "stdafx.h"
#include "IPC/Managed/detail/Interop/Policies/TimeoutFactory.h"


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
        TimeoutFactory::Scheduler::Scheduler(Callback<void()> handler, const TimeoutFactoryFunc& timeoutFatory, const std::chrono::milliseconds& defaultTimeout)
            : m_handler{ std::make_shared<Callback<void()>>(std::move(handler)) },
              m_timeoutFatory{ timeoutFatory },
              m_defaultTimeout{ defaultTimeout }
        {}

        void TimeoutFactory::Scheduler::operator()()
        {
            operator()(m_defaultTimeout);
        }

        void TimeoutFactory::Scheduler::operator()(const std::chrono::milliseconds& timeout)
        {
            operator()(nullptr);

            auto state = m_timeoutFatory([handler = m_handler] { (*handler)(); });
            m_timeoutHandle = std::move(state.first);
            state.second(timeout);
        }

        void TimeoutFactory::Scheduler::operator()(std::nullptr_t)
        {
            m_timeoutHandle = {};
        }


        TimeoutFactory::TimeoutFactory(const std::chrono::milliseconds& defaultTimeout, const TimeoutFactoryFunc& timeoutFatory)
            : m_defaultTimeout{ defaultTimeout != std::chrono::milliseconds::zero() ? defaultTimeout : std::chrono::seconds{ 1 } },
              m_timeoutFatory{ timeoutFatory }
        {}

        auto TimeoutFactory::operator()(Callback<void()> handler) const -> Scheduler
        {
            return{ std::move(handler), m_timeoutFatory, m_defaultTimeout };
        }

    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
