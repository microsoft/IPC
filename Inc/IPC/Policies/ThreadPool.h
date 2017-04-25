#pragma once

#include <memory>


namespace IPC
{
namespace Policies
{
    class ThreadPool
    {
    public:
        ThreadPool();

        ThreadPool(std::uint32_t minThreadCount, std::uint32_t maxThreadCount);

        ThreadPool(const ThreadPool& other) = default;
        ThreadPool& operator=(const ThreadPool& other) = default;

        ThreadPool(ThreadPool&& other) = default;
        ThreadPool& operator=(ThreadPool&& other) = default;

        ~ThreadPool();

        void* GetEnvironment();

    private:
        class Impl;

        std::shared_ptr<Impl> m_impl;
    };

} // Policies
} // IPC
