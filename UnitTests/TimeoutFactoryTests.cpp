#include "stdafx.h"
#include "IPC/Policies/TimeoutFactory.h"
#include <functional>
#include <condition_variable>
#include <mutex>
#include <future>
#include <chrono>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(TimeoutFactoryTests)

static_assert(std::is_copy_constructible<Policies::TimeoutFactory>::value, "TimeoutFactory should be copy constructible.");
static_assert(std::is_copy_assignable<Policies::TimeoutFactory>::value, "TimeoutFactory should be copy assignable.");

BOOST_AUTO_TEST_CASE(InvocationTest)
{
    Policies::TimeoutFactory factory{ std::chrono::milliseconds{ 3 } };

    std::mutex lock;
    bool done = false;
    std::condition_variable cvDone;

    std::function<void()> timeout = factory(
        [&]
        {
            std::lock_guard<std::mutex> guard{ lock };
            done = true;
            cvDone.notify_one();
        });

    timeout();
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvDone.wait(guard, [&] { return done; });
    }
    BOOST_TEST(done);
}

BOOST_AUTO_TEST_CASE(CompletionTest)
{
    Policies::TimeoutFactory factory{ std::chrono::milliseconds{ 3 } };

    std::mutex lock;
    bool processing = false, complete = false;
    std::condition_variable cvProcessing, cvComplete;

    std::function<void()> timeout = factory(
        [&]
        {
            std::unique_lock<std::mutex> guard{ lock };
            processing = true;
            cvProcessing.notify_one();
            cvComplete.wait(guard, [&] { return complete; });
        });

    BOOST_TEST(!!timeout);
    timeout();
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessing.wait(guard, [&] { return processing; });
    }

    auto result = std::async(std::launch::async, [&] { timeout = {}; return true; });

    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 3 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete = true;
        cvComplete.notify_one();
    }
    BOOST_TEST(result.get());
    BOOST_TEST(!timeout);
}

BOOST_AUTO_TEST_CASE(SelfDestructionTest)
{
    Policies::TimeoutFactory factory{ std::chrono::milliseconds{ 3 } };

    std::mutex lock;
    bool done = false;
    std::condition_variable cvDone;

    std::function<void()> timeout = factory(
        [&]
        {
            timeout = {};

            std::lock_guard<std::mutex> guard{ lock };
            done = true;
            cvDone.notify_one();

            return true;
        });

    BOOST_TEST(!!timeout);
    timeout();
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvDone.wait(guard, [&] { return done; });
    }
    BOOST_TEST(!timeout);
}

BOOST_AUTO_TEST_CASE(OneTimeTimeoutTest)
{
    Policies::TimeoutFactory factory;

    std::mutex lock;
    std::size_t count = 0;
    std::condition_variable cvCount;

    std::function<void(const std::chrono::milliseconds&)> timeout = factory(
        [&]
        {
            std::lock_guard<std::mutex> guard{ lock };
            ++count;
            cvCount.notify_one();
        });

    timeout(std::chrono::milliseconds{ 2 });

    BOOST_TEST(!!timeout);
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvCount.wait(guard, [&] { return count == 1; });
    }
    BOOST_TEST(count == 1);
    {
        std::unique_lock<std::mutex> guard{ lock };
        BOOST_TEST(!cvCount.wait_for(guard, std::chrono::milliseconds{ 4 }, [&] { return count == 2; }));
    }
    BOOST_TEST(count == 1);
}

BOOST_AUTO_TEST_CASE(DeactivationTest)
{
    Policies::TimeoutFactory factory;

    std::mutex lock;
    bool processing = false, complete = false;
    std::condition_variable cvProcessing, cvComplete;

    auto timeout = factory(
        [&]
        {
            std::unique_lock<std::mutex> guard{ lock };
            processing = true;
            cvProcessing.notify_one();
            cvComplete.wait(guard, [&] { return complete; });
        });

    timeout(std::chrono::milliseconds{ 2 });
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessing.wait(guard, [&] { return processing; });
    }

    auto result = std::async(std::launch::async, [&] { timeout(nullptr); return true; });
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 3 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete = true;
        cvComplete.notify_one();
    }
    BOOST_TEST(result.get());
}

BOOST_AUTO_TEST_SUITE_END()
