#include "stdafx.h"
#include "IPC/detail/SpinLock.h"
#include <thread>
#include <chrono>


namespace IPC
{
    namespace detail
    {
        void SpinLock::lock()
        {
            std::size_t count{ 0 };
            bool locked{ false };

            while (++count <= c_maxYieldCount)
            {
                if (m_lock.test_and_set(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                else
                {
                    locked = true;
                    break;
                }
            }

            if (!locked)
            {
                std::chrono::milliseconds delay{ c_sleepDuration };

                while (m_lock.test_and_set(std::memory_order_acquire))
                {
                    std::this_thread::sleep_for(delay);
                }
            }
        }

        void SpinLock::unlock()
        {
            m_lock.clear(std::memory_order_release);
        }

    } // detail
} // IPC
