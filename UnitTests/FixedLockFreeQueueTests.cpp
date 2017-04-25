#include "stdafx.h"
#pragma warning(push)
#pragma warning(disable : 4702) // Unreachable code.
#include "IPC/detail/LockFree/FixedQueue.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(FixedLockFreeQueueTests)

static_assert(!std::is_copy_constructible<detail::LockFree::FixedQueue<int, 1>>::value, "LockFree::FixedQueue should not be copy constructible.");
static_assert(!std::is_copy_assignable<detail::LockFree::FixedQueue<int, 1>>::value, "LockFree::FixedQueue should not be copy assignable.");
static_assert(!std::is_move_constructible<detail::LockFree::FixedQueue<int, 1>>::value, "LockFree::FixedQueue should not be move constructible.");
static_assert(!std::is_move_assignable<detail::LockFree::FixedQueue<int, 1>>::value, "LockFree::FixedQueue should not be move assignable.");

BOOST_AUTO_TEST_CASE(TrivialTypeTest)
{
    constexpr std::size_t N = 10;

    detail::LockFree::FixedQueue<std::size_t, N> queue;
    BOOST_TEST(queue.IsEmpty());

    for (std::size_t i = 0; i < N; ++i)
    {
        BOOST_TEST(queue.Push(i + 1));
    }

    BOOST_TEST(!queue.IsEmpty());
    BOOST_TEST(!queue.Push(N));

    auto value = queue.Pop();
    BOOST_TEST(!!value);
    BOOST_TEST(!queue.IsEmpty());
    std::size_t sum = *value;

    BOOST_TEST(queue.ConsumeAll([&](std::size_t&& x) { sum += x; }) == N - 1);
    BOOST_TEST(queue.IsEmpty());

    BOOST_TEST(sum == (N * (N + 1)) / 2);
}

BOOST_AUTO_TEST_CASE(MoveOnlyTypeTest)
{
    detail::LockFree::FixedQueue<std::unique_ptr<int>, 1> queue;
    BOOST_TEST(queue.IsEmpty());

    constexpr int value = 123;
    BOOST_TEST(queue.Push(std::make_unique<int>(value)));
    BOOST_TEST(!queue.IsEmpty());

    auto x = queue.Pop();
    BOOST_TEST(!!x);
    BOOST_TEST(queue.IsEmpty());
    BOOST_TEST(**x == value);
}

BOOST_AUTO_TEST_CASE(ComplexTypeTest)
{
    constexpr std::size_t N = 10;

    detail::LockFree::FixedQueue<std::shared_ptr<int>, N> queue;
    BOOST_TEST(queue.IsEmpty());

    auto value = std::make_shared<int>();
    for (std::size_t i = 0; i < N; ++i)
    {
        BOOST_TEST(queue.Push(value));
    }

    BOOST_TEST(!queue.IsEmpty());
    BOOST_TEST(!queue.Push(value));
    BOOST_TEST(value.use_count() == N + 1);

    BOOST_TEST(queue.ConsumeAll([&](std::shared_ptr<int>&& x) { BOOST_TEST(x == value); }) == N);
    BOOST_TEST(queue.IsEmpty());
    BOOST_TEST(value.unique());
}

BOOST_AUTO_TEST_CASE(ComplexTypeDestructionTest)
{
    auto value = std::make_shared<int>();
    {
        detail::LockFree::FixedQueue<std::shared_ptr<int>, 1> queue;
        BOOST_TEST(queue.IsEmpty());

        BOOST_TEST(queue.Push(value));
        BOOST_TEST(!queue.IsEmpty());
        BOOST_TEST(value.use_count() == 2);
    }
    BOOST_TEST(value.unique());
}

BOOST_AUTO_TEST_CASE(ExceptionSafetyTest)
{
    struct X
    {
        X() = default;

        explicit X(std::shared_ptr<bool> doThrow)
            : m_throw{ std::move(doThrow) }
        {}

        X(const X& other)
        {
            *this = other;
        }

        X& operator=(const X& other)
        {
            if (other.m_throw && *other.m_throw)
            {
                throw std::exception{};
            }

            m_throw = other.m_throw;

            return *this;
        }

        std::shared_ptr<bool> m_throw;
    };

    static_assert(!boost::has_trivial_destructor<X>::value, "X should be a complex type.");

    detail::LockFree::FixedQueue<X, 1> queue;

    auto doThrow = std::make_shared<bool>(true);
    BOOST_CHECK_THROW(queue.Push(X{ doThrow }), std::exception);
    BOOST_TEST(queue.IsEmpty());

    *doThrow = false;
    BOOST_TEST(queue.Push(X{ doThrow }));
    BOOST_TEST(!queue.Push(X{ doThrow }));

    *doThrow = true;
    BOOST_CHECK_THROW(queue.Pop(), std::exception);
    BOOST_TEST(queue.IsEmpty());
    *doThrow = false;
    BOOST_TEST(queue.Push(X{ doThrow }));

    BOOST_CHECK_THROW(queue.ConsumeAll([](X&&) { throw std::exception{}; }), std::exception);
    BOOST_TEST(queue.IsEmpty());
}

BOOST_AUTO_TEST_CASE(StressTest)
{
    constexpr std::size_t N = 100;

    detail::LockFree::FixedQueue<std::unique_ptr<int>, N> queue;
    BOOST_TEST(queue.IsEmpty());

    std::atomic_int value{ 0 }, sum{ 0 };
    std::atomic_size_t count{ 0 };

    constexpr std::size_t ThreadCount = 3;

    std::vector<std::thread> consumers;
    std::vector<std::thread> producers;
    for (std::size_t i = 0; i < ThreadCount; ++i)
    {
       consumers.emplace_back(
            [&]
            {
                while (count != N)
                {
                    if (auto x = queue.Pop())
                    {
                        ++count;
                        sum += **x;
                    }
                    else
                    {
                        std::this_thread::yield();
                    }
                }
            });

        producers.emplace_back(
            [&]
            {
                int x;
                while ((x = ++value) <= N)
                {
                    queue.Push(std::make_unique<int>(x));
                }
                --value;
            });
    }

    for (std::size_t i = 0; i < ThreadCount; ++i)
    {
        producers[i].join();
        consumers[i].join();
    }

    BOOST_TEST(queue.IsEmpty());
    BOOST_TEST(value == N);
    BOOST_TEST(sum == (N * (N + 1)) / 2);
}

BOOST_AUTO_TEST_SUITE_END()

#pragma warning(pop)
