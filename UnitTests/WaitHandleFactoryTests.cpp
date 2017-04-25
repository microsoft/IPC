#include "stdafx.h"
#include "IPC/Policies/WaitHandleFactory.h"
#include "IPC/detail/KernelEvent.h"
#include <condition_variable>
#include <mutex>
#include <future>
#include <atomic>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(WaitHandleFactoryTests)

static_assert(std::is_copy_constructible<Policies::WaitHandleFactory>::value, "WaitHandleFactory should be copy constructible.");
static_assert(std::is_copy_assignable<Policies::WaitHandleFactory>::value, "WaitHandleFactory should be copy assignable.");

BOOST_AUTO_TEST_CASE(InvocationTest)
{
    detail::KernelEvent flag{ create_only, false };
    Policies::WaitHandleFactory factory;

    std::mutex lock;
    std::size_t count = 0;
    std::atomic_bool repeat{ true };
    std::condition_variable cvIncremented;

    auto handle = factory(
        flag,
        [&]
        {
            std::lock_guard<std::mutex> guard{ lock };
            ++count;
            cvIncremented.notify_one();
            return !!repeat;
        });

    BOOST_TEST(handle);

    constexpr std::size_t N = 3;
    for (std::size_t i = 0; i < N; ++i)
    {
        std::unique_lock<std::mutex> guard{ lock };
        BOOST_TEST(flag.Signal());
        cvIncremented.wait(guard, [&] { return count == i + 1; });
        BOOST_TEST(count == i + 1);
    }

    repeat = false;
    {
        std::unique_lock<std::mutex> guard{ lock };
        BOOST_TEST(flag.Signal());
        cvIncremented.wait(guard, [&] { return count == N + 1; });
        BOOST_TEST(count == N + 1);
    }
    {
        std::unique_lock<std::mutex> guard{ lock };
        BOOST_TEST(flag.Signal());
        BOOST_TEST(!cvIncremented.wait_for(guard, std::chrono::milliseconds{ 3 }, [&] { return count == N + 2; }));
    }
}

BOOST_AUTO_TEST_CASE(CompletionTest)
{
    detail::KernelEvent flag{ create_only, false };
    Policies::WaitHandleFactory factory;

    std::mutex lock;
    bool processing = false, complete = false;
    std::condition_variable cvProcessing, cvComplete;

    auto handle = factory(
        flag,
        [&]
        {
            std::unique_lock<std::mutex> guard{ lock };
            processing = true;
            cvProcessing.notify_one();
            cvComplete.wait(guard, [&] { return complete; });
            return true;
        });

    BOOST_TEST(handle);
    BOOST_TEST(flag.Signal());
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessing.wait(guard, [&] { return processing; });
    }

    auto result = std::async(std::launch::async, [&] { handle = {}; return true; });

    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 3 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete = true;
        cvComplete.notify_one();
    }
    BOOST_TEST(result.get());
    BOOST_TEST(!handle);
}

BOOST_AUTO_TEST_CASE(SelfDestructionTest)
{
    detail::KernelEvent flag{ create_only, false };
    Policies::WaitHandleFactory factory;

    std::mutex lock;
    bool done = false;
    std::condition_variable cvDone;

    std::shared_ptr<void> handle = factory(
        flag,
        [&]
        {
            handle = {};

            std::lock_guard<std::mutex> guard{ lock };
            done = true;
            cvDone.notify_one();

            return true;
        });

    BOOST_TEST(handle);
    BOOST_TEST(flag.Signal());
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvDone.wait(guard, [&] { return done; });
    }
    BOOST_TEST(!handle);
}

BOOST_AUTO_TEST_SUITE_END()
