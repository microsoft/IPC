#pragma once

#include "SpinLock.h"
#include <atomic>
#include <thread>


namespace IPC
{
    namespace detail
    {
        class RecursiveSpinLock
        {
        public:
            void lock();

            void unlock();

        private:
            SpinLock m_lock;
            std::atomic<std::thread::id> m_ownerId{};
            volatile std::size_t m_lockCount{ 0 };
        };

    } // detail
} // IPC
