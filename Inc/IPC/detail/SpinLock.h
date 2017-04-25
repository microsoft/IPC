#pragma once

#include <atomic>


namespace IPC
{
    namespace detail
    {
        class SpinLock
        {
        public:
            void lock();

            void unlock();

        private:
            static constexpr std::size_t c_maxYieldCount = 1000;
            static constexpr std::size_t c_sleepDuration = 1;

            std::atomic_flag m_lock{ ATOMIC_FLAG_INIT };
        };

    } // detail
} // IPC
