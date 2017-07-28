#include "stdafx.h"
#include "IPC/Policies/TransactionManager.h"
#include "IPC/detail/Callback.h"
#include "TimeoutFactoryMock.h"
#include <memory>
#include <array>
#include <vector>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(TransactionManagerTests)

static_assert(!std::is_copy_constructible<Policies::TransactionManager<int>>::value, "TransactionManager should not be copy constructible.");
static_assert(!std::is_copy_assignable<Policies::TransactionManager<int>>::value, "TransactionManager should not be copy assignable.");
static_assert(std::is_move_constructible<Policies::TransactionManager<int>>::value, "TransactionManager should be move constructible.");
static_assert(std::is_move_assignable<Policies::TransactionManager<int>>::value, "TransactionManager should be move assignable.");

BOOST_AUTO_TEST_CASE(BeginEndTest)
{
    Policies::TransactionManager<int> transactions;
    constexpr int N = 10;

    std::array<Policies::TransactionManager<int>::Id, N> ids;

    for (int i = 0; i < N; ++i)
    {
        ids[i] = transactions.BeginTransaction(i + 1);
    }

    for (int i = 0; i < N; ++i)
    {
        auto context = transactions.EndTransaction(ids[i]);
        BOOST_TEST(!!context);
        BOOST_TEST(*context == i + 1);
        BOOST_TEST(!transactions.EndTransaction(ids[i]));
    }

    BOOST_TEST(!transactions.EndTransaction(N));
}

BOOST_AUTO_TEST_CASE(TerminationTest)
{
    Policies::TransactionManager<int> transactions;
    constexpr int N = 10;

    std::array<Policies::TransactionManager<int>::Id, N> ids;

    for (int i = 0; i < N; ++i)
    {
        ids[i] = transactions.BeginTransaction(i + 1);
    }

    transactions.TerminateTransactions();

    for (int i = 0; i < N; ++i)
    {
        BOOST_TEST(!transactions.EndTransaction(ids[i]));
    }
}

BOOST_AUTO_TEST_CASE(LifetimeTest)
{
    Policies::TransactionManager<std::shared_ptr<int>> transactions;
    constexpr int N = 10;

    auto obj = std::make_shared<int>();
    std::array<Policies::TransactionManager<int>::Id, N> ids;

    for (int i = 0; i < N; ++i)
    {
        ids[i] = transactions.BeginTransaction(obj);
    }

    BOOST_TEST(obj.use_count() == N + 1);

    for (int i = 0; i < N; ++i)
    {
        {
            auto context = transactions.EndTransaction(ids[i]);
            BOOST_TEST(!!context);
            BOOST_TEST(*context == obj);
        }
        BOOST_TEST(obj.use_count() == N - i);
    }
}

BOOST_AUTO_TEST_CASE(DestructionTest)
{
    auto obj = std::make_shared<int>();

    {
        Policies::TransactionManager<std::shared_ptr<int>> transactions;
        transactions.BeginTransaction(obj);
        BOOST_TEST(obj.use_count() == 2);
    }

    BOOST_TEST(obj.unique());
}

BOOST_AUTO_TEST_CASE(TimeoutTest)
{
    using TimeoutFactory = UnitTest::Mocks::TimeoutFactory;

    TimeoutFactory timeouts;

    Policies::TransactionManager<int, TimeoutFactory> transactions{ timeouts, std::chrono::milliseconds{ 20 } };

    auto id1 = transactions.BeginTransaction(1, std::chrono::milliseconds{ 10 });
    auto id2 = transactions.BeginTransaction(2);

    BOOST_TEST(timeouts->size() == 2);
    auto& t1 = timeouts->front();
    auto& t2 = timeouts->back();

    BOOST_TEST((t1.second == std::chrono::milliseconds{ 10 }));
    BOOST_TEST((t2.second == std::chrono::milliseconds{ 20 }));

    BOOST_TEST(!t1.first.expired());
    (*t1.first.lock())();
    BOOST_TEST(t1.first.expired());
    BOOST_TEST(!transactions.EndTransaction(id1));

    BOOST_TEST(!t2.first.expired());
    (*t2.first.lock())();
    BOOST_TEST(t2.first.expired());
    BOOST_TEST(!transactions.EndTransaction(id2));
}

BOOST_AUTO_TEST_CASE(DefaultTimeoutTest)
{
    UnitTest::Mocks::TimeoutFactory timeouts;
    Policies::TransactionManager<int, decltype(timeouts)> transactions{ timeouts };

    auto check = [&]
    {
        transactions.BeginTransaction(1, std::chrono::milliseconds{ 100 });
        BOOST_TEST(timeouts->size() == 1);
        BOOST_TEST((timeouts->back().second == std::chrono::milliseconds{ 100 }));

        transactions.BeginTransaction(2);
        BOOST_TEST(timeouts->size() == 2);
        BOOST_TEST((timeouts->back().second == transactions.GetDefaultTimeout()));

        transactions.BeginTransaction(3, std::chrono::milliseconds::zero());
        BOOST_TEST(timeouts->size() == 3);
        BOOST_TEST((timeouts->back().second == transactions.GetDefaultTimeout()));
    };

    BOOST_TEST((transactions.GetDefaultTimeout() != std::chrono::milliseconds::zero()));
    check();

    transactions = decltype(transactions){ timeouts, std::chrono::milliseconds{ 10 } };
    timeouts->clear();

    BOOST_TEST((transactions.GetDefaultTimeout() == std::chrono::milliseconds{ 10 }));
    check();
}

BOOST_AUTO_TEST_SUITE_END()
