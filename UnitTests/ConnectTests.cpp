#include "stdafx.h"
#include "IPC/Connect.h"
#include "IPC/Connector.h"
#include "IPC/Acceptor.h"
#include "TraitsMock.h"
#include "TimeoutFactoryMock.h"

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ConnectTests)

struct Traits : UnitTest::Mocks::Traits
{
    using TimeoutFactory = UnitTest::Mocks::TimeoutFactory;
};

BOOST_AUTO_TEST_CASE(SyncClientConnectionTest)
{
    struct Traits : UnitTest::Mocks::Traits
    {
        using WaitHandleFactory = Policies::WaitHandleFactory;
    };

    auto name = detail::GenerateRandomString();

    std::unique_ptr<ServerAcceptor<int, int, Traits>::Connection> connection;
    ServerAcceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { connection = futureConnection.get(); } };

    auto clientAccessor = ConnectClient(name.c_str(), std::make_shared<ClientConnector<int, int, Traits>>(), false);

    BOOST_TEST(!!connection);
    BOOST_TEST(!connection->IsClosed());
    BOOST_TEST(!clientAccessor()->GetConnection().IsClosed());
}

BOOST_AUTO_TEST_CASE(AsyncClientConnectionTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    std::unique_ptr<ServerAcceptor<int, int, Traits>::Connection> connection;
    ServerAcceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { connection = futureConnection.get(); }, channelSettings };

    auto clientAccessor = ConnectClient(
        name.c_str(),
        std::make_shared<ClientConnector<int, int, Traits>>(channelSettings),
        true);

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_CHECK_NO_THROW(clientAccessor());

    BOOST_TEST(!!connection);
    BOOST_TEST(!connection->IsClosed());
    BOOST_TEST(!clientAccessor()->GetConnection().IsClosed());
}

BOOST_AUTO_TEST_CASE(ClientReconnectionTest)
{
    auto name = detail::GenerateRandomString();

    std::unique_ptr<ServerAcceptor<int, int, Traits>::Connection> connection;

    Traits::WaitHandleFactory waitHandleFactory;
    Traits::TimeoutFactory timeoutFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto makeAcceptor = [&]
    {
        return std::make_unique<ServerAcceptor<int, int, Traits>>(
            name.c_str(), [&](auto&& futureConnection) { connection = futureConnection.get(); }, channelSettings);
    };

    auto errorCount = 0;
    auto clientAccessor = ConnectClient(
        name.c_str(),
        std::make_shared<ClientConnector<int, int, Traits>>(channelSettings),
        true,
        timeoutFactory,
        [&](std::exception_ptr) { ++errorCount; });

    constexpr auto IterationCount = 5;
    constexpr auto RetryCount = 5;

    for (auto i = 0; i < IterationCount; ++i)
    {
        if (connection)
        {
            connection.reset();
            BOOST_TEST(waitHandleFactory.Process() != 0);
            BOOST_TEST(timeoutFactory.Process() != 0);
        }

        BOOST_CHECK_THROW(clientAccessor(), std::exception);
        BOOST_TEST(!connection);

        for (auto j = 0; j < RetryCount; ++j)
        {
            auto currentErrorCount = errorCount;
            BOOST_TEST(timeoutFactory.Process() != 0);
            BOOST_TEST(errorCount == currentErrorCount + 1);
        }

        auto acceptor = makeAcceptor();

        BOOST_TEST(timeoutFactory.Process() != 0);
        BOOST_TEST(waitHandleFactory.Process() != 0);

        BOOST_CHECK_NO_THROW(clientAccessor());
        BOOST_TEST(!clientAccessor()->GetConnection().IsClosed());
        BOOST_TEST(!!connection);
        BOOST_TEST(!connection->IsClosed());
    }
}

BOOST_AUTO_TEST_CASE(ConnectingClientDestructionTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    Traits::TimeoutFactory timeoutFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto acceptor = std::make_unique<ServerAcceptor<int, int, Traits>>(name.c_str(), [&](auto&&) {}, channelSettings);

    bool processed = false;
    auto clientAccessor = ConnectClient(
        name.c_str(),
        std::make_shared<ClientConnector<int, int, Traits>>(channelSettings),
        true,
        timeoutFactory,
        [&](std::exception_ptr) { processed = true; });

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(processed);

    auto clientAccessorPtr = std::make_unique<decltype(clientAccessor)>(std::move(clientAccessor));

    processed = false;
    clientAccessorPtr.reset();
    acceptor.reset();

    BOOST_TEST(timeoutFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!processed);
}

BOOST_AUTO_TEST_CASE(SyncServerConnectionTest)
{
    struct Traits : UnitTest::Mocks::Traits
    {
        using WaitHandleFactory = Policies::WaitHandleFactory;
    };

    auto name = detail::GenerateRandomString();

    std::unique_ptr<ClientAcceptor<int, int, Traits>::Connection> connection;
    ClientAcceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { connection = futureConnection.get(); } };

    auto serverAccessor = ConnectServer(
        name.c_str(), std::make_shared<ServerConnector<int, int, Traits>>(), [](auto&&...) { return [](auto&&...) {}; }, false);

    BOOST_TEST(!!connection);
    BOOST_TEST(!connection->IsClosed());
    BOOST_TEST(!serverAccessor()->GetConnection().IsClosed());
}

BOOST_AUTO_TEST_CASE(AsyncServerConnectionTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    std::unique_ptr<ClientAcceptor<int, int, Traits>::Connection> connection;
    ClientAcceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { connection = futureConnection.get(); }, channelSettings };

    auto serverAccessor = ConnectServer(
        name.c_str(), std::make_shared<ServerConnector<int, int, Traits>>(channelSettings), [](auto&&...) { return [](auto&&...) {}; }, true);

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_CHECK_NO_THROW(serverAccessor());

    BOOST_TEST(!!connection);
    BOOST_TEST(!connection->IsClosed());
    BOOST_TEST(!serverAccessor()->GetConnection().IsClosed());
}

BOOST_AUTO_TEST_CASE(ServerReconnectionTest)
{
    auto name = detail::GenerateRandomString();

    std::unique_ptr<ClientAcceptor<int, int, Traits>::Connection> connection;

    Traits::WaitHandleFactory waitHandleFactory;
    Traits::TimeoutFactory timeoutFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto makeAcceptor = [&]
    {
        return std::make_unique<ClientAcceptor<int, int, Traits>>(
            name.c_str(), [&](auto&& futureConnection) { connection = futureConnection.get(); }, channelSettings);
    };

    auto context = std::make_shared<int>();
    auto handler = [context](auto&&...) {};

    auto errorCount = 0;

    auto serverAccessor = ConnectServer(
        name.c_str(),
        std::make_shared<ServerConnector<int, int, Traits>>(channelSettings),
        [handler = std::move(handler)](auto&&...) { return handler; },
        true,
        timeoutFactory,
        [&](std::exception_ptr) { ++errorCount; });

    BOOST_TEST(context.use_count() == 2);

    constexpr auto IterationCount = 5;
    constexpr auto RetryCount = 5;

    for (auto i = 0; i < IterationCount; ++i)
    {
        if (connection)
        {
            connection.reset();
            BOOST_TEST(waitHandleFactory.Process() != 0);
            BOOST_TEST(timeoutFactory.Process() != 0);
        }

        BOOST_CHECK_THROW(serverAccessor(), std::exception);

        BOOST_TEST(context.use_count() == 2);
        BOOST_TEST(!connection);

        for (auto j = 0; j < RetryCount; ++j)
        {
            auto currentErrorCount = errorCount;
            BOOST_TEST(timeoutFactory.Process() != 0);
            BOOST_TEST(errorCount == currentErrorCount + 1);
        }
        
        auto acceptor = makeAcceptor();

        BOOST_TEST(timeoutFactory.Process() != 0);
        BOOST_TEST(waitHandleFactory.Process() != 0);

        BOOST_CHECK_NO_THROW(serverAccessor());
        BOOST_TEST(!serverAccessor()->GetConnection().IsClosed());
        BOOST_TEST(context.use_count() == 3);
        BOOST_TEST(!!connection);
        BOOST_TEST(!connection->IsClosed());
    }

    (void)std::make_unique<decltype(serverAccessor)>(std::move(serverAccessor));
    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(ConnectingServerDestructionTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    Traits::TimeoutFactory timeoutFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto acceptor = std::make_unique<ClientAcceptor<int, int, Traits>>(name.c_str(), [&](auto&&) {}, channelSettings);

    bool processed = false;
    auto serverAccessor = ConnectServer(
        name.c_str(),
        std::make_shared<ServerConnector<int, int, Traits>>(channelSettings),
        [](auto&&...) { return [](auto&&...) {}; },
        true,
        timeoutFactory,
        [&](std::exception_ptr) { processed = true; });

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(processed);

    auto serverAccessorPtr = std::make_unique<decltype(serverAccessor)>(std::move(serverAccessor));

    processed = false;
    serverAccessorPtr.reset();
    acceptor.reset();

    BOOST_TEST(timeoutFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!processed);
}

BOOST_AUTO_TEST_SUITE_END()
