#include "stdafx.h"
#include "IPC/OutputChannel.h"
#include "IPC/InputChannel.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <memory>
#include <bitset>
#include <array>
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>
#include <type_traits>
#include <cassert>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ChannelTests)

constexpr std::size_t c_queueLimit = 20;
constexpr std::size_t c_memSize = 10240;

struct Traits : UnitTest::Mocks::Traits
{
    template <typename T, typename Allocator>
    class Queue : public detail::LockFree::FixedQueue<T, c_queueLimit>
    {
    public:
        explicit Queue(const Allocator& /*allocator*/)
        {}
    };
};

static_assert(!std::is_copy_constructible<OutputChannel<int, Traits>>::value, "OutputChannel should not be copy constructible.");
static_assert(!std::is_copy_assignable<OutputChannel<int, Traits>>::value, "OutputChannel should not be copy assignable.");
static_assert(std::is_move_constructible<OutputChannel<int, Traits>>::value, "OutputChannel should be move constructible.");
static_assert(std::is_move_assignable<OutputChannel<int, Traits>>::value, "OutputChannel should be move assignable.");

BOOST_AUTO_TEST_CASE(ConstructionTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    BOOST_CHECK_NO_THROW((InputChannel<int, Traits>{ create_only, name.c_str(), memory }, OutputChannel<int, Traits>{ open_only, name.c_str(), memory }));
    BOOST_CHECK_NO_THROW((OutputChannel<int, Traits>{ create_only, name.c_str(), memory }, InputChannel<int, Traits>{ open_only, name.c_str(), memory }));
    BOOST_CHECK_THROW((OutputChannel<int, Traits>{ create_only, name.c_str(), memory }, OutputChannel<int, Traits>{ create_only, name.c_str(), memory }), std::exception);
    BOOST_CHECK_THROW((InputChannel<int, Traits>{ create_only, name.c_str(), memory }, InputChannel<int, Traits>{ create_only, name.c_str(), memory }), std::exception);
}

BOOST_AUTO_TEST_CASE(TypeMismatchTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    OutputChannel<int, Traits> out{ create_only, name.c_str(), memory };

    BOOST_CHECK_NO_THROW((InputChannel<int, Traits>{ open_only, name.c_str(), memory }));
    BOOST_CHECK_THROW((InputChannel<float, Traits>{ open_only, name.c_str(), memory }), std::exception);
    BOOST_CHECK_THROW((InputChannel<int, UnitTest::Mocks::Traits>{ open_only, name.c_str(), memory }), std::exception);
}

BOOST_AUTO_TEST_CASE(SyncReceiveTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    OutputChannel<int, Traits> out{ create_only, name.c_str(), memory };
    BOOST_TEST(out.IsEmpty());
    InputChannel<int, Traits> in{ open_only, name.c_str(), memory };
    BOOST_TEST(in.IsEmpty());

    std::bitset<c_queueLimit> bits;
    BOOST_TEST(in.ReceiveAll([&](int i) { assert(!bits.test(i)); bits.set(i); }) == 0);
    BOOST_TEST(bits.none());

    for (int i = 0; i < c_queueLimit; ++i)
    {
        out.Send(i);
    }

    BOOST_TEST(!out.TrySend(0));
    BOOST_CHECK_THROW(out.Send(0), std::exception);

    BOOST_TEST(!in.IsEmpty());
    BOOST_TEST(!out.IsEmpty());

    std::atomic_size_t count{ 0 };
    BOOST_TEST(in.ReceiveAll([&](int i) { assert(!bits.test(i)); bits.set(i); ++count; }) == bits.size());
    BOOST_TEST(bits.all());
    BOOST_TEST(count == c_queueLimit);
    BOOST_TEST(in.IsEmpty());
    BOOST_TEST(out.IsEmpty());
}

BOOST_AUTO_TEST_CASE(AsyncReceiveTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    InputChannel<int, Traits> in{ create_only, name.c_str(), memory, waitHandleFactory };
    BOOST_TEST(in.IsEmpty());
    OutputChannel<int, Traits> out{ open_only, name.c_str(), memory };
    BOOST_TEST(out.IsEmpty());

    std::bitset<c_queueLimit> bits;

    for (int i = 0; i < c_queueLimit/2; ++i)
    {
        out.Send(i);
    }

    std::atomic_size_t count{ 0 };
    BOOST_TEST(in.RegisterReceiver(
        [&](int i)
        {
            assert(!bits.test(i));
            bits.set(i);
            ++count;
        }));

    BOOST_TEST(!in.RegisterReceiver([](int) {}));

    for (int i = c_queueLimit/2; i < c_queueLimit; ++i)
    {
        out.Send(i);
    }

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(bits.all());
    BOOST_TEST(count == c_queueLimit);
    BOOST_TEST(in.IsEmpty());
    BOOST_TEST(out.IsEmpty());

    BOOST_TEST(in.UnregisterReceiver());
    BOOST_TEST(!in.UnregisterReceiver());

    out.Send(0);
    BOOST_TEST(!in.IsEmpty());
    BOOST_TEST(!out.IsEmpty());
}

BOOST_AUTO_TEST_CASE(AsyncReceiveSelfDestructionTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    auto in = std::make_unique<InputChannel<int, Traits>>(create_only, name.c_str(), memory, waitHandleFactory);
    BOOST_TEST(in->IsEmpty());

    std::bitset<c_queueLimit> bits;

    BOOST_TEST(in->RegisterReceiver(
        [&](int i)
        {
            assert(!bits.test(i));
            bits.set(i);
            in.reset();
        }));

    OutputChannel<int, Traits> out{ open_only, name.c_str(), memory };

    for (int i = 0; i < c_queueLimit; ++i)
    {
        out.Send(i);
    }

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(bits.all());
}

BOOST_AUTO_TEST_CASE(LateAsyncReceiveTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    InputChannel<int, Traits> in{ create_only, name.c_str(), memory, waitHandleFactory };
    BOOST_TEST(in.IsEmpty());
    OutputChannel<int, Traits> out{ open_only, name.c_str(), memory };
    BOOST_TEST(out.IsEmpty());

    std::bitset<c_queueLimit> bits;

    for (int i = 0; i < c_queueLimit; ++i)
    {
        out.Send(i);
    }

    BOOST_TEST(in.RegisterReceiver(
        [&](int i)
        {
            assert(!bits.test(i));
            bits.set(i);
        }));

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(bits.all());
    BOOST_TEST(in.IsEmpty());
    BOOST_TEST(out.IsEmpty());
}

BOOST_AUTO_TEST_CASE(MulticastReceiveTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    std::array<bool, c_queueLimit> bits{};
    std::size_t count{ 0 };

    OutputChannel<int, Traits> out1{ create_only, name.c_str(), memory };
    BOOST_TEST(out1.IsEmpty());
    OutputChannel<int, Traits> out2{ open_only, name.c_str(), memory };
    BOOST_TEST(out2.IsEmpty());
    InputChannel<int, Traits> in1{ open_only, name.c_str(), memory, waitHandleFactory };
    BOOST_TEST(in1.IsEmpty());
    InputChannel<int, Traits> in2{ open_only, name.c_str(), memory, waitHandleFactory };
    BOOST_TEST(in2.IsEmpty());

    auto receiver = [&](int i)
    {
        assert(!bits[i]);
        bits[i] = true;
        ++count;
    };

    BOOST_TEST(in1.RegisterReceiver(receiver));
    BOOST_TEST(in2.RegisterReceiver(receiver));

    for (int i = 0; i < c_queueLimit; ++i)
    {
        (i % 2 ? out1 : out2).Send(i);
    }

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(count == c_queueLimit);
    BOOST_TEST(in1.IsEmpty());
    BOOST_TEST(in2.IsEmpty());
    BOOST_TEST(out1.IsEmpty());
    BOOST_TEST(out2.IsEmpty());
}

BOOST_AUTO_TEST_CASE(DestroyedWaitHandlesAfterReceiverUnregistrationTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    InputChannel<int, Traits> in{ create_only, name.c_str(), memory, waitHandleFactory };
    BOOST_TEST(in.IsEmpty());

    bool received = false;

    BOOST_TEST(in.RegisterReceiver([&](int) { received = true; }));

    OutputChannel<int, Traits> out{ open_only, name.c_str(), memory };
    
    out.Send(0);

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(received);

    BOOST_TEST(in.UnregisterReceiver());

    received = false;
    out.Send(1);

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!received);
}

BOOST_AUTO_TEST_CASE(DestroyedWaitHandlesAfterDestructionTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    auto in = std::make_unique<InputChannel<int, Traits>>(create_only, name.c_str(), memory, waitHandleFactory);
    BOOST_TEST(in->IsEmpty());

    bool received = false;

    BOOST_TEST(in->RegisterReceiver([&](int) { received = true; }));

    OutputChannel<int, Traits> out{ open_only, name.c_str(), memory };
    
    out.Send(0);

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(received);

    in.reset();

    received = false;
    out.Send(1);

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!received);
}

BOOST_AUTO_TEST_CASE(MemoryOwnershipTest)
{
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    InputChannel<SharedMemory::UniquePtr<int>, Traits> in{ create_only, name.c_str(), memory };
    BOOST_TEST(in.IsEmpty());
    OutputChannel<SharedMemory::UniquePtr<int>, Traits> out{ open_only, name.c_str(), memory };
    BOOST_TEST(out.IsEmpty());

    out.Send(out.GetMemory()->MakeUnique<int>(anonymous_instance, 777));
    BOOST_TEST(1 == in.ReceiveAll(
        [&](SharedMemory::UniquePtr<int> p)
        {
            BOOST_TEST(p);
            BOOST_TEST(*p == 777);
        }));
}

BOOST_AUTO_TEST_CASE(StressTest)
{
    constexpr std::size_t ThreadCount = 5;

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), ThreadCount * c_memSize);

    InputChannel<int, DefaultTraits> in{ create_only, name.c_str(), memory };
    BOOST_TEST(in.IsEmpty());
    OutputChannel<int, DefaultTraits> out{ open_only, name.c_str(), memory };
    BOOST_TEST(out.IsEmpty());

    std::mutex lock;
    std::condition_variable cvIncremented;

    int sum = 0;
    BOOST_TEST(in.RegisterReceiver(
        [&](int x)
        {
            std::lock_guard<std::mutex> guard{ lock };
            sum += x;
            cvIncremented.notify_one();
        }));

    std::array<std::thread, ThreadCount> threads;

    for (auto& t : threads)
    {
        t = std::thread{ [&]
        {
            for (int i = 0; i < c_queueLimit; ++i)
            {
                while (!out.TrySend(i + 1))
                {
                    std::this_thread::yield();
                }
            }
        } };
    }

    for (auto& t : threads)
    {
        t.join();
    }

    constexpr int c_expectedSum = (ThreadCount * c_queueLimit * (c_queueLimit + 1)) / 2;
    {
        std::unique_lock<std::mutex> guard{ lock };
        cvIncremented.wait(guard, [&] { return sum == c_expectedSum; });
    }
    BOOST_TEST(sum == c_expectedSum);
}

BOOST_AUTO_TEST_SUITE_END()
