#include "stdafx.h"
#include "IPC/Connection.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <type_traits>
#include <memory>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ConnectionTests)

constexpr std::size_t c_queueLimit = 20;
constexpr std::size_t c_memSize = 10240;

struct Traits : UnitTest::Mocks::Traits
{
    template <typename T, typename Allocator>
    class Queue : public detail::LockFree::FixedQueue<T, c_queueLimit>
    {
    public:
        explicit Queue(const Allocator& /*allocator*/)
        {}
    };
};

static_assert(!std::is_copy_constructible<Connection<int, int>>::value, "Connection should not be copy constructible.");
static_assert(!std::is_copy_assignable<Connection<int, int>>::value, "Connection should not be copy assignable.");
static_assert(!std::is_move_constructible<Connection<int, int>>::value, "Connection should not be move constructible.");
static_assert(!std::is_move_assignable<Connection<int, int>>::value, "Connection should not be move assignable.");

BOOST_AUTO_TEST_CASE(ConstructionTest)
{
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);
    detail::KernelEvent closeEvent{ create_only, false };

    BOOST_CHECK_NO_THROW((Connection<int, int, Traits>{ closeEvent, closeEvent, closeEvent, {},
        InputChannel<int, Traits>{ create_only, names.first.c_str(), memory },
        OutputChannel<int, Traits>{ create_only, names.second.c_str(), memory } }));

    BOOST_CHECK_NO_THROW((Connection<int, void, Traits>{ closeEvent, closeEvent, closeEvent, {},
        InputChannel<int, Traits>{ create_only, names.first.c_str(), memory } }));

    BOOST_CHECK_NO_THROW((Connection<void, int, Traits>{ closeEvent, closeEvent, closeEvent, {},
        OutputChannel<int, Traits>{ create_only, names.first.c_str(), memory } }));
}

BOOST_AUTO_TEST_CASE(ChannelAccessTest)
{
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);
    detail::KernelEvent closeEvent{ create_only, false };

    Connection<int, int, Traits> connection{ closeEvent, closeEvent, closeEvent, {},
        InputChannel<int, Traits>{ create_only, names.first.c_str(), memory },
        OutputChannel<int, Traits>{ create_only, names.second.c_str(), memory } };

    BOOST_TEST(!connection.IsClosed());

    BOOST_CHECK_NO_THROW(connection.GetInputChannel());
    BOOST_CHECK_NO_THROW(connection.GetOutputChannel());
    BOOST_TEST(connection.GetInputChannel(std::nothrow) == &connection.GetInputChannel());
    BOOST_TEST(connection.GetOutputChannel(std::nothrow) == &connection.GetOutputChannel());

    connection.Close();
    BOOST_TEST(connection.IsClosed());

    BOOST_CHECK_THROW(connection.GetInputChannel(), std::exception);
    BOOST_CHECK_THROW(connection.GetOutputChannel(), std::exception);
    BOOST_TEST(!connection.GetInputChannel(std::nothrow));
    BOOST_TEST(!connection.GetOutputChannel(std::nothrow));
}

BOOST_AUTO_TEST_CASE(UnmonitoredLocalEventCloseTest)
{
    detail::KernelEvent localEvent{ create_only, true };
    detail::KernelEvent remoteEvent{ create_only, false };
    detail::KernelEvent process{ create_only, false };

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Connection<int, void, Traits> connection{
        localEvent,
        remoteEvent,
        process,
        {},
        InputChannel<int, Traits>{ create_only, name.c_str(), memory } };

    bool closed = false;
    BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(!connection.IsClosed());
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());

    connection.Close();

    BOOST_TEST(connection.IsClosed());
    BOOST_TEST(closed);
    BOOST_TEST(localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());
}

BOOST_AUTO_TEST_CASE(MonitoredLocalEventCloseTest)
{
    detail::KernelEvent localEvent{ create_only, true };
    detail::KernelEvent remoteEvent{ create_only, false };
    detail::KernelEvent process{ create_only, false };

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Connection<int, void, Traits> connection{
        localEvent,
        true,
        remoteEvent,
        process,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory } };

    bool closed = false;
    BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(!connection.IsClosed());
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());

    BOOST_TEST(localEvent.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(connection.IsClosed());
    BOOST_TEST(closed);
    BOOST_TEST(localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());
}

BOOST_AUTO_TEST_CASE(RemoteEventCloseTest)
{
    detail::KernelEvent localEvent{ create_only, false };
    detail::KernelEvent remoteEvent{ create_only, true };
    detail::KernelEvent process{ create_only, false };

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Connection<int, void, Traits> connection{
        localEvent,
        remoteEvent,
        process,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory } };

    bool closed = false;
    BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(!connection.IsClosed());
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());

    BOOST_TEST(remoteEvent.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(connection.IsClosed());
    BOOST_TEST(closed);
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());
}

BOOST_AUTO_TEST_CASE(RemoteProcessCloseTest)
{
    detail::KernelEvent localEvent{ create_only, false };
    detail::KernelEvent remoteEvent{ create_only, false };
    detail::KernelEvent process{ create_only, true };

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Connection<int, void, Traits> connection{
        localEvent,
        remoteEvent,
        process,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory } };

    bool closed = false;
    BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(!connection.IsClosed());
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());

    BOOST_TEST(process.Signal());
    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(connection.IsClosed());
    BOOST_TEST(closed);
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(process.IsSignaled());
}

BOOST_AUTO_TEST_CASE(ClosedRemoteEventConstrutionTest)
{
    detail::KernelEvent localEvent{ create_only, false };
    detail::KernelEvent remoteEvent{ create_only, true, true };
    detail::KernelEvent process{ create_only, false };

    BOOST_TEST(remoteEvent.IsSignaled());

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Connection<int, void, Traits> connection{
        localEvent,
        remoteEvent,
        process,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory } };

    bool closed = false;
    BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(connection.IsClosed());
    BOOST_TEST(closed);
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(remoteEvent.IsSignaled());
    BOOST_TEST(!process.IsSignaled());
}

BOOST_AUTO_TEST_CASE(ClosedRemoteProcessConstrutionTest)
{
    detail::KernelEvent localEvent{ create_only, false };
    detail::KernelEvent remoteEvent{ create_only, false };
    detail::KernelEvent process{ create_only, true, true };

    BOOST_TEST(process.IsSignaled());

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Connection<int, void, Traits> connection{
        localEvent,
        remoteEvent,
        process,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory } };

    bool closed = false;
    BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(connection.IsClosed());
    BOOST_TEST(closed);
    BOOST_TEST(!localEvent.IsSignaled());
    BOOST_TEST(!remoteEvent.IsSignaled());
    BOOST_TEST(process.IsSignaled());
}

BOOST_AUTO_TEST_CASE(CloseHandlerDoubleRegistrationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Connection<void, int, Traits> connection{ closeEvent, closeEvent, closeEvent, {}, OutputChannel<int, Traits>{ create_only, name.c_str(), memory } };

    BOOST_TEST(connection.RegisterCloseHandler([] {}));
    BOOST_TEST(!connection.RegisterCloseHandler([] {}));
    BOOST_TEST(connection.UnregisterCloseHandler());
    BOOST_TEST(!connection.UnregisterCloseHandler());
}

BOOST_AUTO_TEST_CASE(CloseHandlerCombinedRegistrationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Connection<void, int, Traits> connection{ closeEvent, closeEvent, closeEvent, {}, OutputChannel<int, Traits>{ create_only, name.c_str(), memory } };

    bool firstInvoked{ false };
    bool secondInvoked{ false };
    bool thirdInvoked{ false };

    BOOST_TEST(connection.RegisterCloseHandler([&] { firstInvoked = true; }, true));
    BOOST_TEST(connection.RegisterCloseHandler([&] { secondInvoked = true; }, true));
    BOOST_TEST(connection.RegisterCloseHandler([&] { thirdInvoked = true; }, true));

    connection.Close();

    BOOST_TEST(firstInvoked);
    BOOST_TEST(secondInvoked);
    BOOST_TEST(thirdInvoked);
}

BOOST_AUTO_TEST_CASE(CloseHandlerInvocationDuringDestructionTest)
{
    bool closed = false;
    auto closeHandler = [&] { closed = true; };
    {
        detail::KernelEvent closeEvent{ create_only, false };
        auto name = detail::GenerateRandomString();
        auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

        Connection<void, int, Traits> connection{ closeEvent, closeEvent, closeEvent, {}, OutputChannel<int, Traits>{ create_only, name.c_str(), memory } };

        BOOST_TEST(connection.RegisterCloseHandler(closeHandler));
    }
    BOOST_TEST(closed);
}

BOOST_AUTO_TEST_CASE(CloseHandlerSelfDestructionTest)
{
    detail::KernelEvent closeEvent{ create_only, false };

    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    auto connection = std::make_unique<Connection<int, void, Traits>>(
        closeEvent,
        closeEvent,
        closeEvent,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory });

    auto& connectionRef = *connection;
    bool closed = false;

    BOOST_TEST(connectionRef.RegisterCloseHandler(
        [&, connection = std::move(connection)]() mutable
        {
            connection.reset();
            closed = true;
        }));

    BOOST_TEST(closeEvent.Signal());

    BOOST_TEST(waitHandleFactory.Process() != 0);

    BOOST_TEST(closed);
}

BOOST_AUTO_TEST_CASE(CloseHandlerSingleInvocationTest)
{
    std::size_t count = 0;
    auto closeHandler = [&] { ++count; };

    Traits::WaitHandleFactory waitHandleFactory;

    {
        detail::KernelEvent closeEvent{ create_only, true };
        auto name = detail::GenerateRandomString();
        auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

        Connection<int, void, Traits> connection{
            closeEvent,
            closeEvent,
            closeEvent,
            waitHandleFactory,
            InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory } };

        BOOST_TEST(connection.RegisterCloseHandler(closeHandler));

        BOOST_TEST(closeEvent.Signal());
        BOOST_TEST(waitHandleFactory.Process() != 0);
        BOOST_TEST(connection.IsClosed());

        BOOST_TEST(closeEvent.Reset());
        connection.Close();
        BOOST_TEST(waitHandleFactory.Process() == 0);
        BOOST_TEST(connection.IsClosed());

        BOOST_TEST(closeEvent.Signal());
        BOOST_TEST(waitHandleFactory.Process() == 0);
        BOOST_TEST(connection.IsClosed());

        bool closed = false;
        BOOST_TEST(connection.RegisterCloseHandler([&] { closed = true; }));
        BOOST_TEST(closed);
    }

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(count == 1);
}

BOOST_AUTO_TEST_CASE(LocalCloseEventInputChannelUnregistrationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Connection<int, void, Traits> connection{
        closeEvent,
        closeEvent,
        closeEvent,
        waitHandleFactory,
        InputChannel<int, Traits>{ create_only, name.c_str(), memory, waitHandleFactory } };

    int data;
    BOOST_TEST(connection.GetInputChannel().RegisterReceiver([&](int x) { data = x; }));

    OutputChannel<int, Traits> out{ open_only, name.c_str(), memory };
    out.Send(345);

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(data == 345);

    connection.Close();

    data = 0;
    out.Send(678);

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(data == 0);
}

BOOST_AUTO_TEST_SUITE_END()
