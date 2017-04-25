#include "stdafx.h"
#include "IPC/Policies/AsyncReceiverFactory.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

#pragma warning(push)
#include <boost/thread/tss.hpp>
#pragma warning(pop)


namespace IPC
{
namespace Policies
{
    class AsyncReceiverFactory::Scheduler::Impl : public std::enable_shared_from_this<Impl>
    {
    public:
        Impl(detail::Callback<void()> handler, boost::optional<ThreadPool> pool)
            : m_handler{ std::move(handler) },
              m_pool{ std::move(pool) },
              m_work{ ::CreateThreadpoolWork(
                  OnRun, this, m_pool ? static_cast<PTP_CALLBACK_ENVIRON>(m_pool->GetEnvironment()) : nullptr), ::CloseThreadpoolWork }
        {}

        void Stop()
        {
            // The ::WaitForThreadpoolWorkCallbacks cannot be used
            // to wait on pending tasks due to possible recursive calls.

            if (m_recursionDetector.IsExpectedThread())
            {
                m_pendingTasks.Signal();    // Signal extra count here instead of the handler.
            }

            m_pendingTasks.Signal();
            m_pendingTasks.Wait();
        }

        std::size_t operator()()
        {
            m_pendingTasks.AddCount();

            ::SubmitThreadpoolWork(m_work.get());

            return 1;
        }

    private:
        class CountdownEvent
        {
        public:
            void AddCount()
            {
                assert(m_count != 0);
                ++m_count;
            }

            void Signal()
            {
                if (--m_count == 0)
                {
                    m_zero.notify_all();
                }
            }

            auto AutoSignal()
            {
                auto deleter = [this](void*) { Signal(); };

                return std::unique_ptr<void, decltype(deleter)>{ (void*) true, std::move(deleter) };
            }

            void Wait()
            {
                std::unique_lock<decltype(m_lock)> guard{ m_lock };
                m_zero.wait(guard, [&] { return m_count == 0; });
            }

        private:
            std::atomic_size_t m_count{ 1 };
            std::mutex m_lock;
            std::condition_variable m_zero;
        };


        class ThreadDetector
        {
        public:
            bool IsExpectedThread() const
            {
                if (auto isExpected = m_isExpected.get())
                {
                    return *isExpected;
                }

                return false;
            }

            auto Detect()
            {
                bool* isExpected = m_isExpected.get();
                bool prev;

                if (isExpected)
                {
                    prev = *isExpected;
                    *isExpected = true;
                }
                else
                {
                    prev = false;
                    isExpected = new bool{ true };
                    m_isExpected.reset(isExpected);
                }

                auto deleter = [prev, isExpected](void*) { *isExpected = prev; };

                return std::unique_ptr<void, decltype(deleter)>{ (void*) true, std::move(deleter) };
            }

        private:
            boost::thread_specific_ptr<bool> m_isExpected;
        };


        static VOID CALLBACK OnRun(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_WORK /*work*/)
        {
            static_cast<Impl*>(context)->Run();
        }

        void Run()
        {
            auto keepAlive = shared_from_this();
            auto signaler = m_pendingTasks.AutoSignal();
            auto detector = m_recursionDetector.Detect();

            m_handler();
        }


        detail::Callback<void()> m_handler;
        boost::optional<ThreadPool> m_pool;
        std::unique_ptr<TP_WORK, decltype(&::CloseThreadpoolWork)> m_work;
        ThreadDetector m_recursionDetector;
        CountdownEvent m_pendingTasks;
    };


    AsyncReceiverFactory::AsyncReceiverFactory(boost::optional<ThreadPool> pool)
        : m_pool{ std::move(pool) }
    {}

    AsyncReceiverFactory::Scheduler::Scheduler(detail::Callback<void()> handler, boost::optional<ThreadPool> pool)
        : m_impl{ std::make_shared<Impl>(std::move(handler), std::move(pool)) }
    {}

    AsyncReceiverFactory::Scheduler::Scheduler(Scheduler&& other) = default;

    AsyncReceiverFactory::Scheduler::~Scheduler()
    {
        if (m_impl)
        {
            m_impl->Stop();
        }
    }

    std::size_t AsyncReceiverFactory::Scheduler::operator()() const
    {
        return m_impl->operator()();
    }

} // Policies
} // IPC
