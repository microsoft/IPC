#include "stdafx.h"
#include "IPC/detail/RecursiveSpinLock.h"
#include "IPC/Exception.h"
#include <limits>
#include <cassert>


namespace IPC
{
    namespace detail
    {
        void RecursiveSpinLock::lock()
        {
            auto currentId = std::this_thread::get_id();

            if (m_ownerId.load() == currentId)
            {
                if (m_lockCount == (std::numeric_limits<std::size_t>::max)())
                {
                    throw Exception{ "RecursiveSpinLock count overflow." };
                }

                ++m_lockCount;
            }
            else
            {
                m_lock.lock();
                m_ownerId.store(currentId);
                m_lockCount = 1;
            }
        }

        void RecursiveSpinLock::unlock()
        {
            assert(m_ownerId.load() == std::this_thread::get_id());
            assert(m_lockCount != 0);

            if (--m_lockCount == 0)
            {
                m_ownerId.store(std::thread::id{});
                m_lock.unlock();
            }
        }

    } // detail
} // IPC
