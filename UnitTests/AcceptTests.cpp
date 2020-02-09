#include "stdafx.h"
#include "IPC/Accept.h"
#include "IPC/Connector.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <memory>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(AcceptTests)

using Traits = UnitTest::Mocks::Traits;

BOOST_AUTO_TEST_CASE(AcceptedServerAutoRemovalTest)
{
    auto name = detail::GenerateRandomString();
    auto context = std::make_shared<int>();
    auto handler = [context](auto&&...) {};

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto serversAccessor = AcceptServers<int, int, Traits>(
        name.c_str(), [handler = std::move(handler)](auto&&...) { return handler; }, channelSettings);

    BOOST_TEST(context.use_count() == 2);

    ClientConnector<int, int, Traits> connector{ channelSettings };
    {
        auto futureConnection = connector.Connect(name.c_str());

        BOOST_TEST(waitHandleFactory.Process() != 0);

        Client<int, int, Traits> client{ futureConnection.get(), [] {} };
        BOOST_TEST(context.use_count() == 3);
        BOOST_TEST(serversAccessor()->size() == 1);
    }
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(context.use_count() == 2);
    BOOST_TEST(serversAccessor()->empty());

    (void)std::make_unique<decltype(serversAccessor)>(std::move(serversAccessor));
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(AcceptedServerDestructionTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto serversAccessor = AcceptServers<int, int, Traits>(name.c_str(), [](auto&&...) { return [](auto&&...) {}; }, channelSettings);

    auto serversAccessorPtr = std::make_unique<decltype(serversAccessor)>(std::move(serversAccessor));

    ClientConnector<int, int, Traits> connector{ channelSettings };

    bool closed = false;
    auto futureConnection = connector.Connect(name.c_str());

    BOOST_TEST(waitHandleFactory.Process() != 0);

    Client<int, int, Traits> client{ futureConnection.get(), [&] { closed = true; } };

    BOOST_TEST((*serversAccessorPtr)()->size() == 1);

    serversAccessorPtr.reset();

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(closed);
    BOOST_TEST(client.GetConnection().IsClosed());
}

BOOST_AUTO_TEST_CASE(AcceptedServerErrorHandlerInvocationTest)
{
    auto name = detail::GenerateRandomString();

    std::exception_ptr error;
    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto serversAccessor = AcceptServers<int, int, Traits>(
        name.c_str(), [](auto&&...) { return [](auto&&...) {}; }, channelSettings, 0, [&](std::exception_ptr e) { error = e; });

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());

    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };
    BOOST_CHECK_NO_THROW(pingChannel.Send(detail::ConnectorPingInfo{ pingChannel.GetMemory()->GetAllocator<char>() }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(!!error);
}

BOOST_AUTO_TEST_CASE(AcceptedServerDestroyedWaitHandlesAfterDestructionTest)
{
    auto name = detail::GenerateRandomString();

    bool processed = false;
    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto serversAccessor = AcceptServers<int, int, Traits>(
        name.c_str(), [&](auto&&...) { return [&](auto&&...) { processed = true; }; }, channelSettings);

    ClientConnector<int, int, Traits> connector{ channelSettings };

    auto futureConnection = connector.Connect(name.c_str());

    BOOST_TEST(waitHandleFactory.Process() != 0);

    auto client = std::make_unique<Client<int, int, Traits>>(futureConnection.get(), [] {});

    (*client)(0, [](auto&&) {});
    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(processed);

    auto serversAccessorPtr = std::make_unique<decltype(serversAccessor)>(std::move(serversAccessor));

    processed = false;
    serversAccessorPtr.reset();

    (*client)(1, [](auto&&) {});
    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(!processed);

    client.reset();

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!processed);
}

BOOST_AUTO_TEST_CASE(AcceptedClientAutoRemovalTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto clientsAccessor = AcceptClients<int, int, Traits>(name.c_str(), channelSettings);

    ServerConnector<int, int, Traits> connector{ channelSettings };

    {
        auto futureConnection = connector.Connect(name.c_str());

        BOOST_TEST(waitHandleFactory.Process() != 0);

        Server<int, int, Traits> server{ futureConnection.get(), [](int, auto&&) {}, [] {} };
        BOOST_TEST(clientsAccessor()->size() == 1);
    }

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(clientsAccessor()->empty());
}

BOOST_AUTO_TEST_CASE(AcceptedClientDestructionTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto clientsAccessor = AcceptClients<int, int, Traits>(name.c_str(), channelSettings);

    auto clientsAccessorPtr = std::make_unique<decltype(clientsAccessor)>(std::move(clientsAccessor));

    ServerConnector<int, int, Traits> connector{ channelSettings };

    auto futureConnection = connector.Connect(name.c_str());

    BOOST_TEST(waitHandleFactory.Process() != 0);

    bool closed = false;
    Server<int, int, Traits> server{ futureConnection.get(), [](int, auto&&) {}, [&] { closed = true; } };

    BOOST_TEST((*clientsAccessorPtr)()->size() == 1);

    clientsAccessorPtr.reset();

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(closed);
    BOOST_TEST(server.GetConnection().IsClosed());
}

BOOST_AUTO_TEST_CASE(AcceptedClientErrorHandlerInvocationTest)
{
    auto name = detail::GenerateRandomString();

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };
    std::exception_ptr error;

    auto clientsAccessor = AcceptClients<int, int, Traits>(name.c_str(), channelSettings, 0, [&](std::exception_ptr e) { error = e; });

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());

    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };
    BOOST_CHECK_NO_THROW(pingChannel.Send(detail::ConnectorPingInfo{ pingChannel.GetMemory()->GetAllocator<char>() }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(!!error);
}

BOOST_AUTO_TEST_SUITE_END()
