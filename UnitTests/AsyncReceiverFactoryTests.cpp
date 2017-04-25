#include "stdafx.h"
#include "IPC/Policies/AsyncReceiverFactory.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(AsyncReceiverFactoryTests)

struct Queue : std::queue<int>, private std::mutex
{
    boost::optional<int> Pop()
    {
        std::lock_guard<std::mutex> guard{ *this };

        if (empty())
        {
            return boost::none;
        }

        auto x = front();
        pop();
        return x;
    }
};

BOOST_AUTO_TEST_CASE(InvocationTest)
{
    Policies::AsyncReceiverFactory factory;

    Queue queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);

    std::mutex lock;
    std::vector<int> values;
    std::condition_variable cvProcessed;

    auto receiver = factory(
        queue,
        [&](int x)
        {
            std::lock_guard<std::mutex> guard{ lock };
            values.push_back(x);
            cvProcessed.notify_one();
        });

    BOOST_TEST(receiver() == 1);
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessed.wait(guard, [&] { return values.size() == 1; });
        BOOST_TEST(queue.size() == 2);
        BOOST_TEST(values[0] == 1);
    }

    BOOST_TEST(receiver() == 1);
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessed.wait(guard, [&] { return values.size() == 2; });
        BOOST_TEST(queue.size() == 1);
        BOOST_TEST(values[1] == 2);
    }

    BOOST_TEST(receiver() == 1);
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessed.wait(guard, [&] { return values.size() == 3; });
        BOOST_TEST(queue.empty());
        BOOST_TEST(values[2] == 3);
    }
}

BOOST_AUTO_TEST_CASE(BlockedHandlerTest)
{
    Policies::AsyncReceiverFactory factory;

    Queue queue;
    queue.push(0);
    queue.push(0);
    queue.push(0);

    std::mutex lock;
    std::vector<bool> complete;
    std::condition_variable cvProcessing, cvComplete;

    auto receiver = factory(
        queue,
        [&](int)
        {
            std::unique_lock<std::mutex> guard{ lock };
            auto index = complete.size();
            complete.push_back(false);
            cvProcessing.notify_one();
            cvComplete.wait(guard, [&] { return complete[index]; });
        });

    BOOST_TEST(receiver() == 1);
    BOOST_TEST(receiver() == 1);
    BOOST_TEST(receiver() == 1);
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessing.wait(guard, [&] { return complete.size() == 3; });
    }

    auto receiverPtr = std::make_unique<decltype(receiver)>(std::move(receiver));
    auto result = std::async(std::launch::async, [&] { receiverPtr.reset(); return true; });

    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete[2] = true;
        cvComplete.notify_all();
    }
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete[1] = true;
        cvComplete.notify_all();
    }
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete[0] = true;
        cvComplete.notify_all();
    }
    BOOST_TEST(result.get());
    BOOST_TEST(queue.empty());
}

BOOST_AUTO_TEST_CASE(SelfDestructionTest)
{
    Policies::AsyncReceiverFactory factory;

    Queue queue;
    queue.push(123);

    auto state = std::make_shared<int>(0);
    auto& counter = *state;

    std::mutex lock;
    std::condition_variable cvDone;

    std::shared_ptr<void> receiverHandle;
    auto handler = [&, state = std::move(state)](int x)
    {
        std::unique_lock<std::mutex> guard{ lock };
        receiverHandle = {};
        *state = x;
        cvDone.notify_one();
    };

    using Receiver = decltype(factory(queue, handler));

    auto sharedReceiver = std::make_shared<Receiver>(factory(queue, handler));
    auto& receiver = *sharedReceiver;
    receiverHandle = std::move(sharedReceiver);

    BOOST_TEST(receiver() == 1);
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvDone.wait(guard, [&] { return receiverHandle == nullptr && counter == 123; });
    }
    BOOST_TEST(!receiverHandle);
    BOOST_TEST(counter == 123);
}

BOOST_AUTO_TEST_SUITE_END()
