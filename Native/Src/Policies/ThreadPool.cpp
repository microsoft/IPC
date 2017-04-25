#include "stdafx.h"
#include "IPC/Policies/ThreadPool.h"
#include "IPC/Exception.h"


namespace IPC
{
namespace Policies
{
    class ThreadPool::Impl
    {
    public:
        Impl()
            : m_pool{ ::CreateThreadpool(nullptr) }
        {
            if (m_pool == nullptr)
            {
                throw Exception{ "Failed to create thread pool." };
            }

            ::InitializeThreadpoolEnvironment(&m_env);
            ::SetThreadpoolCallbackPool(&m_env, m_pool);
        }

        Impl(std::uint32_t minThreadCount, std::uint32_t maxThreadCount)
            : Impl{}
        {
            if (minThreadCount > maxThreadCount)
            {
                throw Exception{ "The maxThreadCount must be greater or equal to minThreadCount." };
            }

            ::SetThreadpoolThreadMinimum(m_pool, minThreadCount);
            ::SetThreadpoolThreadMaximum(m_pool, maxThreadCount);
        }

        ~Impl()
        {
            ::DestroyThreadpoolEnvironment(&m_env);
            ::CloseThreadpool(m_pool);
        }

        void* GetEnvironment()
        {
            return &m_env;
        }

    private:
        PTP_POOL m_pool;
        TP_CALLBACK_ENVIRON m_env;
    };


    ThreadPool::ThreadPool()
        : m_impl{ std::make_shared<Impl>() }
    {}

    ThreadPool::ThreadPool(std::uint32_t minThreadCount, std::uint32_t maxThreadCount)
        : m_impl{ std::make_shared<Impl>(minThreadCount, maxThreadCount) }
    {}

    ThreadPool::~ThreadPool() = default;

    void* ThreadPool::GetEnvironment()
    {
        return m_impl->GetEnvironment();
    }

} // Policies
} // IPC
