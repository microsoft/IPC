#include "stdafx.h"
#include "IPC/detail/LockFree/Queue.h"

using namespace IPC;


BOOST_AUTO_TEST_SUITE(LockFreeQueueTests)

static_assert(!std::is_copy_constructible<detail::LockFree::Queue<int, std::allocator<void>>>::value, "LockFree::Queue should not be copy constructible.");
static_assert(!std::is_copy_assignable<detail::LockFree::Queue<int, std::allocator<void>>>::value, "LockFree::Queue should not be copy assignable.");
static_assert(!std::is_move_constructible<detail::LockFree::Queue<int, std::allocator<void>>>::value, "LockFree::Queue should not be move constructible.");
static_assert(!std::is_move_assignable<detail::LockFree::Queue<int, std::allocator<void>>>::value, "LockFree::Queue should not be move assignable.");

BOOST_AUTO_TEST_CASE(BasicTest)
{
    constexpr int N = 5;
    detail::LockFree::Queue<int, std::allocator<void>, N/2> queue{ {} };

    BOOST_TEST(queue.IsEmpty());

    for (int i = 0; i < N; ++i)
    {
        BOOST_TEST(queue.Push(i + 1));
    }

    int sum{ 0 };
    for (int i = 0; i < N; ++i)
    {
        auto item = queue.Pop();
        BOOST_TEST(!!item);
        sum += *item;
    }

    BOOST_TEST(sum == N * (N + 1)/2);
}

BOOST_AUTO_TEST_SUITE_END()
