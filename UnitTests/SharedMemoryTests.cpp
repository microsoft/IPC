#include "stdafx.h"
#include "IPC/SharedMemory.h"
#include "IPC/detail/RandomString.h"
#include <mutex>
#include <condition_variable>
#include <future>

#pragma warning(push)
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#pragma warning(pop)

using namespace IPC;


BOOST_AUTO_TEST_SUITE(SharedMemoryTests)

static_assert(!std::is_copy_constructible<SharedMemory>::value, "SharedMemory should not be copy constructible.");
static_assert(!std::is_copy_assignable<SharedMemory>::value, "SharedMemory should not be copy assignable.");
static_assert(std::is_move_constructible<SharedMemory>::value, "SharedMemory should be move constructible.");
static_assert(std::is_move_assignable<SharedMemory>::value, "SharedMemory should be move assignable.");
static_assert(std::is_trivial<SharedMemory::Handle>::value, "SharedMemory::Handle must be a trivial type.");

BOOST_AUTO_TEST_CASE(SharedMemoryConstructTest)
{
    auto name = detail::GenerateRandomString();

    BOOST_CHECK_THROW((SharedMemory{ create_only, name.c_str(), SharedMemory::GetMinSize() / 2 }), std::exception);
    BOOST_CHECK_NO_THROW((SharedMemory{ create_only, name.c_str(), 1024 }, SharedMemory{ open_only, name.c_str() }));
}

BOOST_AUTO_TEST_CASE(SharedMemoryNameTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    BOOST_TEST(name == m1.GetName());
    BOOST_TEST(name == m2.GetName());
}

BOOST_AUTO_TEST_CASE(ObjectConstructionTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto& x = m1.Construct<int>(anonymous_instance, 123);
    BOOST_TEST(x == 123);
    auto& y = m1.Construct<int>(anonymous_instance, 456);
    BOOST_TEST(y == 456);
    BOOST_CHECK_NO_THROW(m1.Destruct(&x));
    BOOST_CHECK_NO_THROW(m1.Destruct(&y));

    auto& pair1 = m1.Construct<std::pair<int, int>>(unique_instance, 33, 55);
    BOOST_TEST((pair1 == std::make_pair(33, 55)));
    BOOST_CHECK_THROW((m1.Construct<std::pair<int, int>>(unique_instance, 44, 66)), std::exception);
    BOOST_CHECK_THROW((m2.Construct<std::pair<int, int>>(unique_instance, 33, 77)), std::exception);
    auto& pair2 = m2.Find<std::pair<int, int>>(unique_instance);
    BOOST_TEST(&pair1 != &pair2);
    BOOST_TEST((pair1 == pair2));
    std::swap(pair1.first, pair1.second);
    BOOST_TEST((pair1 == pair2));
    BOOST_CHECK_NO_THROW(m1.Destruct(&pair1));
    BOOST_CHECK_THROW((m1.Find<std::pair<int, int>>(unique_instance)), std::exception);
    BOOST_CHECK_THROW((m2.Find<std::pair<int, int>>(unique_instance)), std::exception);

    auto& named1 = m1.Construct<int>("X", 999);
    BOOST_TEST(named1 == 999);
    BOOST_CHECK_NO_THROW(m1.Find<float>("X"));
    BOOST_CHECK_NO_THROW(m2.Find<float>("X"));
    auto& named2 = m2.Find<int>("X");
    BOOST_TEST(&named1 != &named2);
    BOOST_TEST(named1 == named2);
    named1 = 101;
    BOOST_TEST(named1 == named2);
    BOOST_CHECK_NO_THROW(m1.Destruct(&named1));
    BOOST_CHECK_THROW(m1.Find<int>("X"), std::exception);
    BOOST_CHECK_THROW(m2.Find<int>("X"), std::exception);
}

BOOST_AUTO_TEST_CASE(ObjectDestructionTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    static int s_count = 0;

    struct X
    {
        X() { ++s_count; }
        ~X() { --s_count; }
    };

    BOOST_TEST(s_count == 0);
    auto& x1 = m1.Construct<X>("X");
    BOOST_TEST(s_count == 1);
    auto& x2 = m2.Find<X>("X");
    BOOST_TEST(&x1 != &x2);
    BOOST_CHECK_NO_THROW(m2.Destruct(&x2));
    BOOST_CHECK_THROW(m1.Find<X>("X"), std::exception);
    BOOST_CHECK_THROW(m2.Find<X>("X"), std::exception);
    BOOST_TEST(s_count == 0);
}

BOOST_AUTO_TEST_CASE(UniquePtrTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto& x1 = m1.Construct<int>("X", 111);
    BOOST_TEST(x1 == 111);
    {
        auto p = m1.MakeUniquePtr(&x1);
        auto& x2 = m2.Find<int>("X");
        BOOST_TEST(&x1 != &x2);
        BOOST_TEST(x1 == x2);
    }
    BOOST_CHECK_THROW(m1.Find<int>("X"), std::exception);
    BOOST_CHECK_THROW(m2.Find<int>("X"), std::exception);

    {
        SharedMemory::UniquePtr<int> p = m1.MakeUnique<int>("Y", 222);
        BOOST_TEST(p);
        BOOST_TEST(*p == 222);
        auto& y2 = m2.Find<int>("Y");
        BOOST_TEST(p.get() != &y2);
        BOOST_TEST(*p == y2);
    }
    BOOST_CHECK_THROW(m1.Find<int>("Y"), std::exception);
    BOOST_CHECK_THROW(m2.Find<int>("Y"), std::exception);
}

BOOST_AUTO_TEST_CASE(SharedPtrTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto& x1 = m1.Construct<int>("X", 321);
    BOOST_TEST(x1 == 321);
    {
        auto p = m1.MakeSharedPtr(&x1);
        auto q = p;
        BOOST_TEST(p == q);
        BOOST_TEST(p.use_count() == 2);
        auto& x2 = m2.Find<int>("X");
        BOOST_TEST(&x1 != &x2);
        BOOST_TEST(x1 == x2);
    }
    BOOST_CHECK_THROW(m1.Find<int>("X"), std::exception);
    BOOST_CHECK_THROW(m2.Find<int>("X"), std::exception);

    {
        SharedMemory::SharedPtr<int> p = m1.MakeShared<int>("Y", 222);
        BOOST_TEST(p);
        BOOST_TEST(*p == 222);
        auto q = p;
        BOOST_TEST(p == q);
        BOOST_TEST(p.use_count() == 2);
        auto& y2 = m2.Find<int>("Y");
        BOOST_TEST(p.get() != &y2);
        BOOST_TEST(*p == y2);
    }
    BOOST_CHECK_THROW(m1.Find<int>("Y"), std::exception);
    BOOST_CHECK_THROW(m2.Find<int>("Y"), std::exception);
}

BOOST_AUTO_TEST_CASE(WeakPtrTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto& x1 = m1.Construct<int>("X", 444);
    BOOST_TEST(x1 == 444);
    {
        auto p = m1.MakeSharedPtr(&x1);
        auto w = m1.MakeWeakPtr<int>(p);
        BOOST_TEST(p == w.lock());
        BOOST_TEST(p.unique());
        auto& x2 = m2.Find<int>("X");
        BOOST_TEST(&x1 != &x2);
        BOOST_TEST(x1 == x2);
    }
    BOOST_CHECK_THROW(m1.Find<int>("X"), std::exception);
    BOOST_CHECK_THROW(m2.Find<int>("X"), std::exception);
}

BOOST_AUTO_TEST_CASE(HandleTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto& x1 = m1.Construct<int>(anonymous_instance, 100);
    BOOST_TEST(x1 == 100);
    auto handle = m1.ToHandle(x1);
    auto& x2 = m2.FromHandle<int>(handle);
    BOOST_TEST(&x1 != &x2);
    BOOST_TEST(x1 == x2);
    x2 = 333;
    BOOST_TEST(x1 == x2);
    BOOST_CHECK_NO_THROW(m1.Destruct(&x1));
}

BOOST_AUTO_TEST_CASE(AllocatorTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    using String = boost::interprocess::basic_string<char, std::char_traits<char>, SharedMemory::Allocator<char>>;

    auto& s1 = m1.Construct<String>("S", name.c_str(), m1.GetAllocator<char>());
    BOOST_TEST(s1 == name.c_str());
    auto& s2 = m2.Find<String>("S");
    BOOST_TEST(&s1 != &s2);
    BOOST_TEST(s1 == s2);
    BOOST_CHECK_NO_THROW(m1.Destruct(&s1));
    BOOST_CHECK_THROW(m1.Find<String>("S"), std::exception);
    BOOST_CHECK_THROW(m2.Find<String>("S"), std::exception);
}

BOOST_AUTO_TEST_CASE(AllocatorDefautInitTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m{ create_only, name.c_str(), 1024 };

    boost::interprocess::vector<int, SharedMemory::Allocator<int>> v{ 100, boost::container::default_init, m.GetAllocator<int>() };
    BOOST_TEST(v.size() == 100);
}

BOOST_AUTO_TEST_CASE(DeleterTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    static int s_count = 0;

    struct X
    {
        X() { ++s_count; }
        ~X() { --s_count; }
    };

    BOOST_TEST(s_count == 0);
    auto& x1 = m1.Construct<X>("X");
    BOOST_TEST(s_count == 1);
    auto& x2 = m2.Find<X>("X");
    BOOST_TEST(&x1 != &x2);

    std::unique_ptr<X, SharedMemory::Deleter<X>>{ &x2, m2.GetDeleter<X>() };

    BOOST_CHECK_THROW(m1.Find<X>("X"), std::exception);
    BOOST_CHECK_THROW(m2.Find<X>("X"), std::exception);
    BOOST_TEST(s_count == 0);
}

BOOST_AUTO_TEST_CASE(InMemorySmartPtrTest)
{
    struct X
    {
        X(SharedMemory& m)
            : up{ m.MakeUnique<int>(anonymous_instance, 100) },
              sp{ m.MakeShared<int>(anonymous_instance, 200) },
              wp{ sp }
        {}

        SharedMemory::UniquePtr<int> up;
        SharedMemory::SharedPtr<int> sp;
        SharedMemory::WeakPtr<int> wp;
    };

    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    auto& x1 = m1.Construct<X>(unique_instance, m1);
    auto& x2 = m2.Find<X>(unique_instance);

    BOOST_TEST(&x1 != &x2);
    BOOST_TEST(*x1.up == 100);
    BOOST_TEST(*x1.up == *x2.up);
    BOOST_TEST(*x1.sp == 200);
    BOOST_TEST(*x1.sp == *x2.sp);
    BOOST_TEST(x1.sp.unique());
    BOOST_TEST(x1.sp.use_count() == x2.sp.use_count());
    BOOST_TEST(x1.sp == x1.wp.lock());
    BOOST_TEST(x2.sp == x2.wp.lock());
    BOOST_CHECK_NO_THROW(m2.Destruct(&x2));
    BOOST_CHECK_THROW(m1.Find<X>(unique_instance), std::exception);
    BOOST_CHECK_THROW(m2.Find<X>(unique_instance), std::exception);
}

BOOST_AUTO_TEST_CASE(InvokeAtomicTest)
{
    auto name = detail::GenerateRandomString();
    SharedMemory m1{ create_only, name.c_str(), 1024 };
    SharedMemory m2{ open_only, name.c_str() };

    std::mutex lock;
    bool processing = false, complete = false;
    std::condition_variable cvProcessing, cvComplete;

    auto r1 = std::async(std::launch::async,
        [&]
        {
            m1.InvokeAtomic([&]
            {
                std::unique_lock<std::mutex> guard{ lock };
                processing = true;
                cvProcessing.notify_one();
                cvComplete.wait(guard, [&] { return complete; });
            });
            return true;
        });

    {
        std::unique_lock<std::mutex> guard{ lock };
        cvProcessing.wait(guard, [&] { return processing; });
    }

    auto r2 = std::async(std::launch::async,
        [&]
        {
            m1.MakeUnique<int>(unique_instance);
            return true;
        });

    BOOST_TEST((r1.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
    BOOST_TEST((r2.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
    {
        std::lock_guard<std::mutex> guard{ lock };
        complete = true;
        cvComplete.notify_one();
    }
    BOOST_TEST(r1.get());
    BOOST_TEST(r2.get());
}

BOOST_AUTO_TEST_SUITE_END()
