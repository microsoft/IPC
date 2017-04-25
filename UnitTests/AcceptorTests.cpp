#include "stdafx.h"
#include "IPC/Acceptor.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <memory>
#include <type_traits>
#include <future>
#include <vector>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)

using namespace IPC;


BOOST_AUTO_TEST_SUITE(AcceptorTests)

static_assert(!std::is_copy_constructible<Acceptor<int, int>>::value, "Acceptor should not be copy constructible.");
static_assert(!std::is_copy_assignable<Acceptor<int, int>>::value, "Acceptor should not be copy assignable.");
static_assert(std::is_move_constructible<Acceptor<int, int>>::value, "Acceptor should be move constructible.");
static_assert(std::is_move_assignable<Acceptor<int, int>>::value, "Acceptor should be move assignable.");

using Traits = UnitTest::Mocks::Traits;

BOOST_AUTO_TEST_CASE(HostInfoTest)
{
    auto name = detail::GenerateRandomString();

    std::string acceptorCloseEventName;
    {
        Acceptor<int, int, Traits> acceptor{ name.c_str(), [](auto&&) {} };

        auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
        auto& info = memory->Find<detail::AcceptorHostInfo>(unique_instance);

        BOOST_TEST(info.m_processId == detail::KernelProcess::GetCurrentProcessId());
        BOOST_CHECK_NO_THROW((OutputChannel<detail::ConnectorPingInfo, Traits>{ open_only, nullptr, memory }));
        BOOST_CHECK_NO_THROW((detail::KernelEvent{ open_only, info.m_acceptorCloseEventName.c_str() }));

        acceptorCloseEventName.assign(info.m_acceptorCloseEventName.c_str());
    }
    BOOST_CHECK_THROW((SharedMemory{ open_only, name.c_str() }), std::exception);
    BOOST_CHECK_THROW((detail::KernelEvent{ open_only, acceptorCloseEventName.c_str() }), std::exception);
}

BOOST_AUTO_TEST_CASE(DestructionNotificationTest)
{
    boost::optional<detail::KernelEvent> acceptorCloseEvent;
    {
        auto name = detail::GenerateRandomString();

        Acceptor<int, int, Traits> acceptor{ name.c_str(), [&](auto&&) {} };

        auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
        auto& info = memory->Find<detail::AcceptorHostInfo>(unique_instance);

        acceptorCloseEvent.emplace(open_only, info.m_acceptorCloseEventName.c_str());
        BOOST_TEST(!acceptorCloseEvent->IsSignaled());
    }
    BOOST_TEST(acceptorCloseEvent->IsSignaled());
}

BOOST_AUTO_TEST_CASE(InvalidPingInfoTest)
{
    auto name = detail::GenerateRandomString();

    std::vector<std::future<std::unique_ptr<Acceptor<int, int, Traits>::Connection>>> connections;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    Acceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { connections.push_back(std::move(futureConnection)); }, channelSettings };

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    detail::ConnectorPingInfo correctPing{ pingChannel.GetMemory()->GetAllocator<char>() };
    correctPing.m_processId = detail::KernelProcess::GetCurrentProcessId();
    correctPing.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
    correctPing.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
    correctPing.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    detail::KernelEvent connectorCloseEvent{ create_only, false, false, correctPing.m_connectorCloseEventName.c_str() };
    auto acceptorInfoChannel = channels.CreateInput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        correctPing.m_acceptorInfoChannelName.c_str());
    auto connectorInfoChannel = channels.CreateOutput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        correctPing.m_connectorInfoChannelName.c_str());

    constexpr auto invalidProcessId = 0;
    const auto invalidName = detail::GenerateRandomString();

    std::vector<detail::ConnectorPingInfo> pings(4, correctPing);
    pings[0].m_processId = invalidProcessId;
    pings[1].m_connectorCloseEventName.assign(invalidName.c_str());
    pings[2].m_acceptorInfoChannelName.assign(invalidName.c_str());
    pings[3].m_connectorInfoChannelName.assign(invalidName.c_str());

    for (auto& ping : pings)
    {
        const bool correctCloseEvent = (ping.m_connectorCloseEventName == correctPing.m_connectorCloseEventName);

        BOOST_CHECK_NO_THROW(pingChannel.Send(std::move(ping)));
        BOOST_TEST(waitHandleFactory.Process() != 0);

        if (correctCloseEvent)
        {
            BOOST_TEST(connectorCloseEvent.IsSignaled());
            connectorCloseEvent.Reset();
        }
    }

    BOOST_TEST(connections.size() == pings.size());

    for (auto& connection : connections)
    {
        BOOST_CHECK_THROW(connection.get(), std::exception);
    }
}

BOOST_AUTO_TEST_CASE(InvalidConnectorInfoTest)
{
    auto name = detail::GenerateRandomString();

    std::future<std::unique_ptr<Acceptor<int, int, Traits>::Connection>> connection;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    Acceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { connection = std::move(futureConnection); }, channelSettings };

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    detail::ConnectorPingInfo ping{ pingChannel.GetMemory()->GetAllocator<char>() };
    ping.m_processId = detail::KernelProcess::GetCurrentProcessId();
    ping.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
    ping.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
    ping.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    detail::KernelEvent connectorCloseEvent{ create_only, false, false, ping.m_connectorCloseEventName.c_str() };
    auto acceptorInfoChannel = channels.CreateInput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        ping.m_acceptorInfoChannelName.c_str());
    auto connectorInfoChannel = channels.CreateOutput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        ping.m_connectorInfoChannelName.c_str());

    const auto invalidName = detail::GenerateRandomString();
    detail::ConnectorInfo connectorInfo{ connectorInfoChannel.GetMemory()->GetAllocator<char>() };
    connectorInfo.m_channelName.assign(invalidName.c_str());

    BOOST_CHECK_NO_THROW(pingChannel.Send(std::move(ping)));
    BOOST_CHECK_NO_THROW(connectorInfoChannel.Send(
        detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>{ Traits::PacketId{ 1 }, connectorInfo }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(connection.valid());

    BOOST_CHECK_THROW(connection.get(), std::exception);
}

BOOST_AUTO_TEST_CASE(ClosedConnectorTest)
{
    auto name = detail::GenerateRandomString();

    bool connected = false;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    Acceptor<int, int, Traits> acceptor{name.c_str(), [&](auto&&) { connected = true; }, channelSettings };

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    auto& info = memory->Find<detail::AcceptorHostInfo>(unique_instance);

    detail::KernelEvent acceptorCloseEvent{ open_only, info.m_acceptorCloseEventName.c_str() };
    BOOST_TEST(!acceptorCloseEvent.IsSignaled());

    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    detail::ConnectorPingInfo ping{ pingChannel.GetMemory()->GetAllocator<char>() };
    ping.m_processId = detail::KernelProcess::GetCurrentProcessId();
    ping.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
    ping.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
    ping.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    detail::KernelEvent connectorCloseEvent{ create_only, false, false, ping.m_connectorCloseEventName.c_str() };
    auto acceptorInfoChannel = channels.CreateInput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        ping.m_acceptorInfoChannelName.c_str());
    auto connectorInfoChannel = channels.CreateOutput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        ping.m_connectorInfoChannelName.c_str());

    Traits::PacketId id{};
    BOOST_CHECK_NO_THROW(pingChannel.Send(std::move(ping)));
    BOOST_CHECK_NO_THROW(connectorInfoChannel.Send(detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>{
        ++id, detail::ConnectorInfo{ connectorInfoChannel.GetMemory()->GetAllocator<char>() } }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(connected);

    connected = false;
    BOOST_TEST(connectorCloseEvent.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_CHECK_NO_THROW(connectorInfoChannel.Send(detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>{
        ++id, detail::ConnectorInfo{ connectorInfoChannel.GetMemory()->GetAllocator<char>() } }));

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(!connected);

    BOOST_TEST(!acceptorCloseEvent.IsSignaled());
}

BOOST_AUTO_TEST_CASE(DuplexConnectionConstructionTest)
{
    auto name = detail::GenerateRandomString();

    std::future<std::unique_ptr<Acceptor<int, int, Traits>::Connection>> result;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    Acceptor<int, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { result = std::move(futureConnection); }, channelSettings };

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    detail::ConnectorPingInfo ping{ pingChannel.GetMemory()->GetAllocator<char>() };
    ping.m_processId = detail::KernelProcess::GetCurrentProcessId();
    ping.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
    ping.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
    ping.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    detail::KernelEvent connectorCloseEvent{ create_only, false, false, ping.m_connectorCloseEventName.c_str() };
    auto acceptorInfoChannel = channels.CreateInput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        ping.m_acceptorInfoChannelName.c_str());
    auto connectorInfoChannel = channels.CreateOutput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        ping.m_connectorInfoChannelName.c_str());

    detail::ConnectorInfo connectorInfo{ connectorInfoChannel.GetMemory()->GetAllocator<char>() };
    connectorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());

    auto out = channels.CreateOutput<int>(connectorInfo.m_channelName.c_str());

    BOOST_CHECK_NO_THROW(pingChannel.Send(std::move(ping)));
    BOOST_CHECK_NO_THROW(connectorInfoChannel.Send(
        detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>{ Traits::PacketId{ 1 }, connectorInfo }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(result.valid());

    boost::optional<detail::AcceptorInfo> acceptorInfo;
    BOOST_TEST(1 == (acceptorInfoChannel.ReceiveAll([&](auto&& packet) { acceptorInfo = std::move(packet.GetPayload()); })));
    BOOST_TEST(!!acceptorInfo);

    detail::KernelEvent closeEvent{ open_only, acceptorInfo->m_closeEventName.c_str() };
    auto in = channels.OpenInput<int>(acceptorInfo->m_channelName.c_str());

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

    std::future<std::unique_ptr<Acceptor<void, int, Traits>::Connection>> result;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    Acceptor<void, int, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { result = std::move(futureConnection); }, channelSettings };

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    detail::ConnectorPingInfo ping{ pingChannel.GetMemory()->GetAllocator<char>() };
    ping.m_processId = detail::KernelProcess::GetCurrentProcessId();
    ping.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
    ping.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
    ping.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    detail::KernelEvent connectorCloseEvent{ create_only, false, false, ping.m_connectorCloseEventName.c_str() };
    auto acceptorInfoChannel = channels.CreateInput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        ping.m_acceptorInfoChannelName.c_str());
    auto connectorInfoChannel = channels.CreateOutput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        ping.m_connectorInfoChannelName.c_str());

    BOOST_CHECK_NO_THROW(pingChannel.Send(std::move(ping)));
    BOOST_CHECK_NO_THROW(connectorInfoChannel.Send(detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>{
        Traits::PacketId{ 1 }, detail::ConnectorInfo{ connectorInfoChannel.GetMemory()->GetAllocator<char>() } }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(result.valid());

    boost::optional<detail::AcceptorInfo> acceptorInfo;
    BOOST_TEST(1 == (acceptorInfoChannel.ReceiveAll([&](auto&& packet) { acceptorInfo = std::move(packet.GetPayload()); })));
    BOOST_TEST(!!acceptorInfo);

    detail::KernelEvent closeEvent{ open_only, acceptorInfo->m_closeEventName.c_str() };
    auto in = channels.OpenInput<int>(acceptorInfo->m_channelName.c_str());

    auto connection = result.get();
    BOOST_TEST(!connection->IsClosed());

    BOOST_CHECK_NO_THROW(connection->GetOutputChannel().Send(300));
    BOOST_TEST(1 == (in.ReceiveAll([](int x) { BOOST_TEST(x == 300); })));

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

    std::future<std::unique_ptr<Acceptor<int, void, Traits>::Connection>> result;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    Acceptor<int, void, Traits> acceptor{
        name.c_str(), [&](auto&& futureConnection) { result = std::move(futureConnection); }, channelSettings };

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    detail::ConnectorPingInfo ping{ pingChannel.GetMemory()->GetAllocator<char>() };
    ping.m_processId = detail::KernelProcess::GetCurrentProcessId();
    ping.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
    ping.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
    ping.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

    auto channels = detail::ChannelFactory<Traits>{ channelSettings }.MakeInstance();

    detail::KernelEvent connectorCloseEvent{ create_only, false, false, ping.m_connectorCloseEventName.c_str() };
    auto acceptorInfoChannel = channels.CreateInput<detail::ResponsePacket<detail::AcceptorInfo, Traits::PacketId>>(
        ping.m_acceptorInfoChannelName.c_str());
    auto connectorInfoChannel = channels.CreateOutput<detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>>(
        ping.m_connectorInfoChannelName.c_str());

    detail::ConnectorInfo connectorInfo{ connectorInfoChannel.GetMemory()->GetAllocator<char>() };
    connectorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());

    auto out = channels.CreateOutput<int>(connectorInfo.m_channelName.c_str());

    BOOST_CHECK_NO_THROW(pingChannel.Send(std::move(ping)));
    BOOST_CHECK_NO_THROW(connectorInfoChannel.Send(
        detail::RequestPacket<detail::ConnectorInfo, Traits::PacketId>{ Traits::PacketId{ 1 }, connectorInfo }));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(result.valid());

    boost::optional<detail::AcceptorInfo> acceptorInfo;
    BOOST_TEST(1 == (acceptorInfoChannel.ReceiveAll([&](auto&& packet) { acceptorInfo = std::move(packet.GetPayload()); })));
    BOOST_TEST(!!acceptorInfo);

    detail::KernelEvent closeEvent{ open_only, acceptorInfo->m_closeEventName.c_str() };
    BOOST_CHECK_THROW(channels.OpenInput<int>(acceptorInfo->m_channelName.c_str()), std::exception);

    auto connection = result.get();
    BOOST_TEST(!connection->IsClosed());

    BOOST_CHECK_NO_THROW(out.Send(400));
    BOOST_TEST(1 == (connection->GetInputChannel().ReceiveAll([](int x) { BOOST_TEST(x == 400); })));

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

    bool processed = false;

    Traits::WaitHandleFactory waitHandleFactory;
    ChannelSettings<Traits> channelSettings{ waitHandleFactory };

    auto acceptor = std::make_unique<Acceptor<int, int, Traits>>(
        name.c_str(), [&](auto&&) { processed = true; }, channelSettings);

    auto memory = std::make_shared<SharedMemory>(open_only, name.c_str());
    auto& info = memory->Find<detail::AcceptorHostInfo>(unique_instance);

    detail::KernelEvent acceptorCloseEvent{ open_only, info.m_acceptorCloseEventName.c_str() };
    BOOST_TEST(!acceptorCloseEvent.IsSignaled());

    OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, memory };

    BOOST_CHECK_NO_THROW(pingChannel.Send(detail::ConnectorPingInfo{ pingChannel.GetMemory()->GetAllocator<char>() }));
    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(processed);

    processed = false;
    acceptor.reset();

    BOOST_CHECK_NO_THROW(pingChannel.Send(detail::ConnectorPingInfo{ pingChannel.GetMemory()->GetAllocator<char>() }));
    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!processed);

    BOOST_TEST(acceptorCloseEvent.IsSignaled());
}

BOOST_AUTO_TEST_SUITE_END()
