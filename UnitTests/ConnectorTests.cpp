#include "stdafx.h"
#include "IPC/Connector.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <type_traits>
#include <vector>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ConnectorTests)

static_assert(!std::is_copy_constructible<Connector<int, int>>::value, "Connector should not be copy constructible.");
static_assert(!std::is_copy_assignable<Connector<int, int>>::value, "Connector should not be copy assignable.");
static_assert(std::is_move_constructible<Connector<int, int>>::value, "Connector should be move constructible.");
static_assert(std::is_move_assignable<Connector<int, int>>::value, "Connector should be move assignable.");

using Traits = UnitTest::Mocks::Traits;

BOOST_AUTO_TEST_CASE(MissingHostInfoTest)
{
    auto name = detail::GenerateRandomString();

    Connector<int, int, Traits> connector;

    BOOST_CHECK_THROW(connector.Connect(name.c_str()).get(), std::exception);
}

BOOST_AUTO_TEST_CASE(InvalidHostInfoTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    auto correctInfo = hostInfo;
    correctInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    correctInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, correctInfo.m_acceptorCloseEventName.c_str() };

    constexpr auto invalidProcessId = 0;
    const auto invalidName = detail::GenerateRandomString();

    std::vector<detail::AcceptorHostInfo> infos(2, correctInfo);
    infos[0].m_processId = invalidProcessId;
    infos[1].m_acceptorCloseEventName.assign(invalidName.c_str());

    Connector<int, int, Traits> connector{ channelSettings };

    for (auto& info : infos)
    {
        hostInfo = info;
        BOOST_CHECK_THROW(connector.Connect(name.c_str()).get(), std::exception);
    }

    BOOST_TEST(acceptorCloseEvent.Signal());
    hostInfo = correctInfo;

    auto futureConnection = connector.Connect(name.c_str());
    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_CHECK_THROW(futureConnection.get(), std::exception);
}

BOOST_AUTO_TEST_CASE(PingInfoVerificationTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    Connector<int, int, Traits> connector{ channelSettings };

    bool invoked = false;
    connector.Connect(name.c_str(), [&](auto&&) { invoked = true; });

    boost::optional<detail::ConnectorPingInfo> pingInfo;
    BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info){ pingInfo = std::move(info); })));

    BOOST_TEST(waitHandleFactory.Process() == 0);

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    BOOST_CHECK_NO_THROW((detail::KernelProcess{ pingInfo->m_processId }));
    BOOST_CHECK_NO_THROW((detail::KernelEvent{ open_only, pingInfo->m_connectorCloseEventName.c_str() }));
    BOOST_CHECK_NO_THROW((channels.OpenInput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        pingInfo->m_connectorInfoChannelName.c_str())));
    BOOST_CHECK_NO_THROW((channels.OpenOutput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        pingInfo->m_acceptorInfoChannelName.c_str())));

    BOOST_TEST(!invoked);
}

BOOST_AUTO_TEST_CASE(InvalidAcceptorInfoTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    Connector<int, int, Traits> connector{ channelSettings };

    connector.Connect(name.c_str(), [](auto&&) {});

    boost::optional<detail::ConnectorPingInfo> pingInfo;
    BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info) { pingInfo = std::move(info); })));

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    auto connectorInfoChannel = channels.OpenInput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        pingInfo->m_connectorInfoChannelName.c_str());
    auto acceptorInfoChannel = channels.OpenOutput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        pingInfo->m_acceptorInfoChannelName.c_str());

    BOOST_TEST(waitHandleFactory.Process() == 0);

    BOOST_TEST(1 == (connectorInfoChannel.ReceiveAll([](auto&&) {})));

    detail::AcceptorInfo correctAcceptorInfo{ acceptorInfoChannel.GetMemory()->GetAllocator<char>() };
    correctAcceptorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());
    correctAcceptorInfo.m_closeEventName.assign(detail::GenerateRandomString().c_str());

    auto out = channels.CreateOutput<int>(correctAcceptorInfo.m_channelName.c_str());
    detail::KernelEvent closeEvent{ create_only, false, false, correctAcceptorInfo.m_closeEventName.c_str() };

    const auto invalidName = detail::GenerateRandomString();

    std::vector<detail::AcceptorInfo> infos(2, correctAcceptorInfo);
    infos[0].m_closeEventName.assign(invalidName.c_str());
    infos[1].m_channelName.assign(invalidName.c_str());

    std::vector<std::future<std::unique_ptr<Connector<int, int, Traits>::Connection>>> connections;

    for (auto& info : infos)
    {
        connector.Connect(name.c_str(), [&](auto&& futureConnection) { connections.push_back(std::move(futureConnection)); });

        BOOST_TEST(pingChannel.IsEmpty());

        BOOST_TEST(1 == (connectorInfoChannel.ReceiveAll(
            [&](auto&& packet)
            {
                BOOST_CHECK_NO_THROW(acceptorInfoChannel.Send(
                    detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>{ packet.GetId(), std::move(info) }));
            })));
    }

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(connections.size() == infos.size());

    for (auto& connection : connections)
    {
        BOOST_CHECK_THROW(connection.get(), std::exception);
    }
    BOOST_TEST(closeEvent.IsSignaled());
}

BOOST_AUTO_TEST_CASE(ClosedAcceptorTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    Connector<int, int, Traits> connector{ channelSettings };

    {
        auto result = connector.Connect(name.c_str());
        BOOST_TEST(1 == (pingChannel.ReceiveAll([](detail::ConnectorPingInfo&&) {})));

        BOOST_TEST(acceptorCloseEvent.Signal());
        BOOST_TEST(waitHandleFactory.Process() != 0);

        BOOST_CHECK_THROW(result.get(), std::exception);
    }

    {
        auto result = connector.Connect(name.c_str());
        boost::optional<detail::ConnectorPingInfo> pingInfo;
        BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info) { pingInfo = std::move(info); })));
        BOOST_TEST(!!pingInfo);

        detail::KernelEvent connectorCloseEvent{ open_only, pingInfo->m_connectorCloseEventName.c_str() };

        BOOST_TEST(connectorCloseEvent.Signal());
        BOOST_TEST(waitHandleFactory.Process() != 0);

        BOOST_CHECK_THROW(result.get(), std::exception);
    }

    {
        std::future<std::unique_ptr<Connector<int, int, Traits>::Connection>> connection;
        connector.Connect(name.c_str(), [&](auto&& futureConnection) { connection = std::move(futureConnection); });

        BOOST_TEST(!connection.valid());

        BOOST_TEST(acceptorCloseEvent.Signal());
        BOOST_TEST(waitHandleFactory.Process() != 0);

        BOOST_TEST(connection.valid());
        BOOST_CHECK_THROW(connection.get(), std::exception);
    }
}

BOOST_AUTO_TEST_CASE(DuplexConnectionConstructionTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    Connector<int, int, Traits> connector{ channelSettings };

    auto result = connector.Connect(name.c_str());

    boost::optional<detail::ConnectorPingInfo> pingInfo;
    BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info) { pingInfo = std::move(info); })));

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    auto connectorInfoChannel = channels.OpenInput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        pingInfo->m_connectorInfoChannelName.c_str());
    auto acceptorInfoChannel = channels.OpenOutput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        pingInfo->m_acceptorInfoChannelName.c_str());

    detail::AcceptorInfo acceptorInfo{ acceptorInfoChannel.GetMemory()->GetAllocator<char>() };
    acceptorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());
    acceptorInfo.m_closeEventName.assign(detail::GenerateRandomString().c_str());

    auto out = channels.CreateOutput<int>(acceptorInfo.m_channelName.c_str());
    detail::KernelEvent closeEvent{ create_only, false, false, acceptorInfo.m_closeEventName.c_str() };

    boost::optional<detail::ConnectorInfo> connectorInfo;
    BOOST_TEST(1 == (connectorInfoChannel.ReceiveAll(
        [&](auto&& packet)
        {
            connectorInfo = std::move(packet.GetPayload());
            BOOST_CHECK_NO_THROW(acceptorInfoChannel.Send(
                detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>{ packet.GetId(), std::move(acceptorInfo) }));
        })));

    auto in = channels.OpenInput<int>(connectorInfo->m_channelName.c_str());

    BOOST_TEST(waitHandleFactory.Process() != 0);

    auto connection = result.get();
    BOOST_TEST(!connection->IsClosed());

    BOOST_CHECK_NO_THROW(out.Send(100));
    BOOST_TEST(1 == (connection->GetInputChannel().ReceiveAll([](int x) { BOOST_TEST(x == 100); })));

    BOOST_CHECK_NO_THROW(connection->GetOutputChannel().Send(200));
    BOOST_TEST(1 == (in.ReceiveAll([](int x) { BOOST_TEST(x == 200); })));

    bool closed = false;
    BOOST_TEST(connection->RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(closeEvent.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(closed);
    BOOST_TEST(connection->IsClosed());
}

BOOST_AUTO_TEST_CASE(NoInputConnectionConstructionTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    Connector<void, int, Traits> connector{ channelSettings };

    auto result = connector.Connect(name.c_str());

    boost::optional<detail::ConnectorPingInfo> pingInfo;
    BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info) { pingInfo = std::move(info); })));

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    auto connectorInfoChannel = channels.OpenInput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        pingInfo->m_connectorInfoChannelName.c_str());
    auto acceptorInfoChannel = channels.OpenOutput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        pingInfo->m_acceptorInfoChannelName.c_str());

    detail::AcceptorInfo acceptorInfo{ acceptorInfoChannel.GetMemory()->GetAllocator<char>() };
    acceptorInfo.m_closeEventName.assign(detail::GenerateRandomString().c_str());

    detail::KernelEvent closeEvent{ create_only, false, false, acceptorInfo.m_closeEventName.c_str() };

    boost::optional<detail::ConnectorInfo> connectorInfo;
    BOOST_TEST(1 == (connectorInfoChannel.ReceiveAll(
        [&](auto&& packet)
        {
            connectorInfo = std::move(packet.GetPayload());
            BOOST_CHECK_NO_THROW(acceptorInfoChannel.Send(
                detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>{ packet.GetId(), std::move(acceptorInfo) }));
        })));

    auto in = channels.OpenInput<int>(connectorInfo->m_channelName.c_str());

    BOOST_TEST(waitHandleFactory.Process() != 0);

    auto connection = result.get();
    BOOST_TEST(!connection->IsClosed());

    BOOST_CHECK_NO_THROW(connection->GetOutputChannel().Send(200));
    BOOST_TEST(1 == (in.ReceiveAll([](int x) { BOOST_TEST(x == 200); })));

    bool closed = false;
    BOOST_TEST(connection->RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(closeEvent.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(closed);
    BOOST_TEST(connection->IsClosed());
}

BOOST_AUTO_TEST_CASE(NoOutputConnectionConstructionTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    Connector<int, void, Traits> connector{ channelSettings };

    auto result = connector.Connect(name.c_str());

    boost::optional<detail::ConnectorPingInfo> pingInfo;
    BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info) { pingInfo = std::move(info); })));

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    auto connectorInfoChannel = channels.OpenInput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        pingInfo->m_connectorInfoChannelName.c_str());
    auto acceptorInfoChannel = channels.OpenOutput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        pingInfo->m_acceptorInfoChannelName.c_str());

    detail::AcceptorInfo acceptorInfo{ acceptorInfoChannel.GetMemory()->GetAllocator<char>() };
    acceptorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());
    acceptorInfo.m_closeEventName.assign(detail::GenerateRandomString().c_str());

    auto out = channels.CreateOutput<int>(acceptorInfo.m_channelName.c_str());
    detail::KernelEvent closeEvent{ create_only, false, false, acceptorInfo.m_closeEventName.c_str() };

    boost::optional<detail::ConnectorInfo> connectorInfo;
    BOOST_TEST(1 == (connectorInfoChannel.ReceiveAll(
        [&](auto&& packet)
        {
            connectorInfo = std::move(packet.GetPayload());
            BOOST_CHECK_NO_THROW(acceptorInfoChannel.Send(
                detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>{ packet.GetId(), std::move(acceptorInfo) }));
        })));

    BOOST_CHECK_THROW(channels.OpenInput<int>(connectorInfo->m_channelName.c_str()), std::exception);

    BOOST_TEST(waitHandleFactory.Process() != 0);

    auto connection = result.get();
    BOOST_TEST(!connection->IsClosed());

    BOOST_CHECK_NO_THROW(out.Send(100));
    BOOST_TEST(1 == (connection->GetInputChannel().ReceiveAll([](int x) { BOOST_TEST(x == 100); })));

    bool closed = false;
    BOOST_TEST(connection->RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(closeEvent.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(closed);
    BOOST_TEST(connection->IsClosed());
}

BOOST_AUTO_TEST_CASE(DestroyedWaitHandlesAfterDestructionTest)
{
    auto name = detail::GenerateRandomString();

    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 32 * 1024);
    auto& hostInfo = memory->Construct<detail::AcceptorHostInfo>(unique_instance, memory->GetAllocator<char>());

    hostInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
    hostInfo.m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    InputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ create_only, nullptr, memory, waitHandleFactory };
    detail::KernelEvent acceptorCloseEvent{ create_only, false, false, hostInfo.m_acceptorCloseEventName.c_str() };

    auto connector = std::make_unique<Connector<int, int, Traits>>(channelSettings);

    bool processed = false;
    connector->Connect(name.c_str(), [&](auto&&) { processed = true; });

    boost::optional<detail::ConnectorPingInfo> pingInfo;
    BOOST_TEST(1 == (pingChannel.ReceiveAll([&](detail::ConnectorPingInfo&& info) { pingInfo = std::move(info); })));

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    auto connectorInfoChannel = channels.OpenInput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        pingInfo->m_connectorInfoChannelName.c_str());
    auto acceptorInfoChannel = channels.OpenOutput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        pingInfo->m_acceptorInfoChannelName.c_str());

    BOOST_TEST(1 == (connectorInfoChannel.ReceiveAll(
        [&](auto&& packet)
        {
            BOOST_CHECK_NO_THROW(acceptorInfoChannel.Send(detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>{
                packet.GetId(),
                detail::AcceptorInfo{ acceptorInfoChannel.GetMemory()->GetAllocator<char>() } }));
        })));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(processed);

    processed = false;
    connector.reset();

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!processed);
}

BOOST_AUTO_TEST_SUITE_END()
