#include "stdafx.h"
#include "IPC/detail/SharedObject.h"
#include "IPC/detail/RandomString.h"
#include <memory>
#include <mutex>
#include <future>
#include <chrono>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(SharedObjectTests)

static_assert(std::is_copy_constructible<detail::SharedObject<int>>::value, "SharedObject should be copy constructible.");
static_assert(std::is_copy_assignable<detail::SharedObject<int>>::value, "SharedObject should be copy assignable.");

BOOST_AUTO_TEST_CASE(OwnershipTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto owner = std::make_unique<detail::SharedObject<int>>(create_only, m1, "X", 333);
    BOOST_CHECK_THROW((detail::SharedObject<int>{ create_only, m1, "X", 333 }), std::exception);
    BOOST_TEST(**owner == 333);
    BOOST_TEST(owner->use_count() == 2);

    SharedMemory::SharedPtr<int> x1 = detail::SharedObject<int>{ open_only, m1, "X" };
    BOOST_TEST(*x1 == 333);
    BOOST_TEST(owner->use_count() == 3);
    BOOST_TEST(x1.use_count() == 3);
    BOOST_TEST(*owner == x1);

    SharedMemory::SharedPtr<int> x2 = detail::SharedObject<int>{ open_only, m2, "X" };
    BOOST_TEST(*x2 == 333);
    BOOST_TEST(owner->use_count() == 4);
    BOOST_TEST(x1.use_count() == 4);
    BOOST_TEST(x2.use_count() == 4);
    BOOST_TEST(*owner != x2);
    BOOST_TEST(x1 != x2);

    owner.reset();

    BOOST_CHECK_THROW((detail::SharedObject<int>{ open_only, m1, "X" }), std::exception);
    BOOST_CHECK_NO_THROW((detail::SharedObject<int>{ create_only, m1, "X", 333 }));
    BOOST_TEST(x1.use_count() == 2);
    BOOST_TEST(x2.use_count() == 2);
}

BOOST_AUTO_TEST_CASE(ThreadSafetyTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    static std::mutex s_ctorLock;

    struct X
    {
        X()
        {
            std::lock_guard<std::mutex>{ s_ctorLock };
        }
    };

    s_ctorLock.lock();

    std::unique_ptr<detail::SharedObject<X>> owner;
    auto r1 = std::async(std::launch::async,
        [&]
        {
            owner = std::make_unique<detail::SharedObject<X>>(create_only, m1, "X");
            assert(owner->use_count() >= 2);
        });

    auto r2 = std::async(std::launch::async,
        [&]
        {
            try
            {
                detail::SharedObject<X> x1{ open_only, m1, "X" };
                assert(x1.use_count() >= 1);
            }
            catch (const std::exception&)
            { }
        });

    auto r3 = std::async(std::launch::async,
        [&]
        {
            try
            {
                detail::SharedObject<X> x2{ open_only, m2, "X" };
                assert(x2.use_count() >= 1);
            }
            catch (const std::exception&)
            { }
        });

    s_ctorLock.unlock();
    r1.wait();
    BOOST_TEST(!!owner);

    r2.wait();
    r3.wait();
    BOOST_TEST(owner->use_count() == 2);
}

BOOST_AUTO_TEST_SUITE_END()
