#include "stdafx.h"
#include "IPC/detail/SpinLock.h"
#include "IPC/detail/RecursiveSpinLock.h"
#include <future>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(SpinLockTests)

BOOST_AUTO_TEST_CASE(LockTest)
{
    detail::SpinLock lock;

    lock.lock();

    auto result = std::async(std::launch::async, [&] { lock.lock(); return true; });
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));

    lock.unlock();

    BOOST_TEST(result.get());
}

BOOST_AUTO_TEST_CASE(RecursiveLockTest)
{
    detail::RecursiveSpinLock lock;

    lock.lock();

    auto result = std::async(std::launch::async, [&] { lock.lock(); return true; });
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));

    lock.lock();
    lock.unlock();

    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
    lock.unlock();

    BOOST_TEST(result.get());
}

BOOST_AUTO_TEST_SUITE_END()
