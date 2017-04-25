#include "stdafx.h"
#include "IPC/Policies/TimeoutFactory.h"
#include <thread>
#include <atomic>
#include <cassert>


namespace IPC
{
namespace Policies
{
    class TimeoutFactory::Timeout : public std::enable_shared_from_this<Timeout>
    {
    public:
        template <typename Function>
        Timeout(boost::optional<ThreadPool> pool, Function&& func, const std::chrono::milliseconds& defaultTimeout)
            : m_func{ std::forward<Function>(func) },
              m_pool{ std::move(pool) },
              m_timer{ ::CreateThreadpoolTimer(
                  OnTimer, this, m_pool ? static_cast<PTP_CALLBACK_ENVIRON>(m_pool->GetEnvironment()) : nullptr), ::CloseThreadpoolTimer },
              m_defaultTimeout{ defaultTimeout }
        {}

        void Reset()
        {
            Reset(m_defaultTimeout);
        }

        void Reset(const std::chrono::milliseconds& timeout)
        {
            Deactivate();

            FILETIME actualTimeout;
            ConvertTimeout(timeout, actualTimeout);

            Reset(const_cast<LPFILETIME>(&actualTimeout));
        }

        void Deactivate()
        {
            if (::IsThreadpoolTimerSet(m_timer.get()))
            {
                Reset(nullptr);

                if (m_handlerThreadId != std::this_thread::get_id())
                {
                    ::WaitForThreadpoolTimerCallbacks(m_timer.get(), TRUE);
                }
            }
        }

    private:
        static VOID CALLBACK OnTimer(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_TIMER /*timer*/)
        {
            static_cast<Timeout*>(context)->HandleEvent();
        }

        void HandleEvent()
        {
            auto keepAlive = shared_from_this();

            m_handlerThreadId = std::this_thread::get_id();

            m_func();

            m_handlerThreadId = std::thread::id{};
        }

        void Reset(LPFILETIME ft) const
        {
            ::SetThreadpoolTimer(m_timer.get(), ft, 0, 0);
        }

        static void ConvertTimeout(const std::chrono::milliseconds& from, FILETIME& to)
        {
            LARGE_INTEGER li;
            li.QuadPart = -(static_cast<LONG64>(from.count()) * 10000LL);
            to.dwLowDateTime = li.LowPart;
            to.dwHighDateTime = li.HighPart;
        }


        detail::Callback<void()> m_func;
        boost::optional<ThreadPool> m_pool;
        std::unique_ptr<TP_TIMER, decltype(&::CloseThreadpoolTimer)> m_timer;
        std::atomic<std::thread::id> m_handlerThreadId{};
        std::chrono::milliseconds m_defaultTimeout;
    };


    TimeoutFactory::Scheduler::Scheduler(std::shared_ptr<Timeout> timeout)
        : m_timeout{ std::move(timeout) }
    {}

    TimeoutFactory::Scheduler::~Scheduler()
    {
        if (m_timeout)
        {
            m_timeout->Deactivate();
        }
    }

    void TimeoutFactory::Scheduler::operator()() const
    {
        m_timeout->Reset();
    }

    void TimeoutFactory::Scheduler::operator()(const std::chrono::milliseconds& timeout) const
    {
        m_timeout->Reset(timeout);
    }

    void TimeoutFactory::Scheduler::operator()(std::nullptr_t) const
    {
        m_timeout->Deactivate();
    }


    TimeoutFactory::TimeoutFactory(const std::chrono::milliseconds& defaultTimeout, boost::optional<ThreadPool> pool)
        : m_defaultTimeout{ defaultTimeout != std::chrono::milliseconds::zero() ? defaultTimeout : std::chrono::seconds{ 1 } },
          m_pool{ std::move(pool) }
    {}

    auto TimeoutFactory::operator()(detail::Callback<void()> handler) const -> Scheduler
    {
        return Scheduler{ std::make_shared<Timeout>(m_pool, std::move(handler), m_defaultTimeout) };
    }

} // Policies
} // IPC
