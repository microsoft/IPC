#include "stdafx.h"
#include "IPC/Policies/WaitHandleFactory.h"
#include "IPC/detail/KernelObject.h"
#include "IPC/Exception.h"
#include <atomic>
#include <thread>


namespace IPC
{
namespace Policies
{
    class WaitHandleFactory::Waiter : public std::enable_shared_from_this<Waiter>
    {
    public:
        template <typename Function>
        Waiter(boost::optional<ThreadPool> pool, detail::KernelObject obj, Function&& func)
            : m_func{ std::forward<Function>(func) },
              m_pool{ std::move(pool) },
              m_wait{ ::CreateThreadpoolWait(
                  OnEvent, this, m_pool ? static_cast<PTP_CALLBACK_ENVIRON>(m_pool->GetEnvironment()) : nullptr), ::CloseThreadpoolWait },
              m_obj{ std::move(obj) }
        {}

        void Monitor()
        {
            Monitor(static_cast<void*>(m_obj));
        }

        void Stop()
        {
            m_stop = true;

            if (m_handlerThreadId != std::this_thread::get_id())
            {
                ::WaitForThreadpoolWaitCallbacks(m_wait.get(), TRUE);   // Wait for threads that did not observe m_stop.
                Monitor(nullptr);
                ::WaitForThreadpoolWaitCallbacks(m_wait.get(), TRUE);   // Wait for threads that did observe m_stop.
            }
            else
            {
                Monitor(nullptr);
            }
        }

    private:
        static VOID CALLBACK OnEvent(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_WAIT /*wait*/, TP_WAIT_RESULT waitResult)
        {
            if (waitResult == WAIT_OBJECT_0)
            {
                static_cast<Waiter*>(context)->HandleEvent();
            }
            else
            {
                throw Exception{ "Unexpected wait result is received." };
            }
        }

        void Monitor(HANDLE handle)
        {
            ::SetThreadpoolWait(m_wait.get(), handle, nullptr);
        }

        void HandleEvent()
        {
            auto keepAlive = shared_from_this();

            m_handlerThreadId = std::this_thread::get_id();

            auto monitor = m_func() && !m_stop;     // The m_stop must be checked last for recursive case.

            m_handlerThreadId = std::thread::id{};

            if (monitor)
            {
                Monitor();
            }
        }


        detail::Callback<bool()> m_func;
        boost::optional<ThreadPool> m_pool;
        std::unique_ptr<TP_WAIT, decltype(&::CloseThreadpoolWait)> m_wait;
        detail::KernelObject m_obj;
        std::atomic_bool m_stop{ false };
        std::atomic<std::thread::id> m_handlerThreadId{};
    };


    WaitHandleFactory::WaitHandleFactory(boost::optional<ThreadPool> pool)
        : m_pool{ std::move(pool) }
    {}

    std::shared_ptr<void> WaitHandleFactory::operator()(detail::KernelObject obj, detail::Callback<bool()> handler) const
    {
        auto waiter = std::make_shared<Waiter>(m_pool, std::move(obj), std::move(handler));

        waiter->Monitor();

        return{ (void*) true, [waiter = std::move(waiter)](void*) { waiter->Stop(); } };
    }

} // Policies
} // IPC
