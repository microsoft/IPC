#include "stdafx.h"
#include "IPC/detail/LockFree/IndexedObjectPool.h"

using namespace IPC;


BOOST_AUTO_TEST_SUITE(LockFreeIndexedObjectPoolTests)

static_assert(!std::is_copy_constructible<detail::LockFree::IndexedObjectPool<int, std::allocator<void>>>::value, "LockFree::IndexedObjectPool should not be copy constructible.");
static_assert(!std::is_copy_assignable<detail::LockFree::IndexedObjectPool<int, std::allocator<void>>>::value, "LockFree::IndexedObjectPool should not be copy assignable.");
static_assert(!std::is_move_constructible<detail::LockFree::IndexedObjectPool<int, std::allocator<void>>>::value, "LockFree::IndexedObjectPool should not be move constructible.");
static_assert(!std::is_move_assignable<detail::LockFree::IndexedObjectPool<int, std::allocator<void>>>::value, "LockFree::IndexedObjectPool should not be move assignable.");

BOOST_AUTO_TEST_CASE(BasicTest)
{
    struct Object
    {
        explicit Object(std::uint32_t index, int x = 0)
            : m_value{ index + x }
        {
            if (x < 0)
            {
                throw std::exception{};
            }
        }

        std::uint32_t m_value;
    };

    detail::LockFree::IndexedObjectPool<Object, std::allocator<void>, 2> pool{ {} };

    BOOST_CHECK_THROW(pool.Take(-1), std::exception);

    std::uint32_t i = 0;
    for (auto item : { pool.Take(1u), pool.Take(2u), pool.Take(3u), pool.Take(4u), pool.Take(5u) })
    {
        BOOST_TEST(item.first.m_value == 2*(i + 1));
        ++i;
        item.first.m_value *= 10u;
        BOOST_TEST(pool.Return(item.second, [&](auto& x) { BOOST_TEST(&item.first == &x); }));
        BOOST_TEST(!pool.Return(item.second));
    }

    BOOST_TEST(!pool.Return(5));
    BOOST_TEST(!pool.Return(10));

    {
        bool invoked = false;
        pool.ReturnAll([&](auto&) { invoked = true; });
        BOOST_TEST(!invoked);
    }

    constexpr auto c_expectedSum = (2*1) * 10u + (2*2) * 10 + (2*3) * 10 + (2*4) * 10 + (2*5) * 10;

    {
        std::uint32_t sum = 0;
        for (auto item : { pool.Take(), pool.Take(), pool.Take(), pool.Take(), pool.Take() })
        {
            sum += item.first.m_value;
            BOOST_TEST(pool.Return(item.second, [&](auto& x) { BOOST_TEST(&item.first == &x); }));
            BOOST_TEST(!pool.Return(item.second));
        }
        BOOST_TEST(sum == c_expectedSum);
    }

    {
        std::uint32_t sum = 0;

        pool.Take(), pool.Take(), pool.Take(), pool.Take(), pool.Take();
        pool.ReturnAll([&](auto& o) { sum += o.m_value; });

        BOOST_TEST(sum == c_expectedSum);

        bool invoked = false;
        pool.ReturnAll([&](auto&) { invoked = true; });
        BOOST_TEST(!invoked);
    }
}

BOOST_AUTO_TEST_SUITE_END()
