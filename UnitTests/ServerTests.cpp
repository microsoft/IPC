#include "stdafx.h"
#include "IPC/Server.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <type_traits>
#include <memory>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ServerTests)

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

static_assert(!std::is_copy_constructible<Server<int, int>>::value, "Server should not be copy constructible.");
static_assert(!std::is_copy_assignable<Server<int, int>>::value, "Server should not be copy assignable.");
static_assert(!std::is_move_constructible<Server<int, int>>::value, "Server should not be move constructible.");
static_assert(!std::is_move_assignable<Server<int, int>>::value, "Server should not be move assignable.");

BOOST_AUTO_TEST_CASE(HandlerNoResponseInvocationTest)
{
    bool invoked = false, closed = false;
    {
        int data;
        detail::KernelEvent closeEvent{ create_only, false };
        auto name = detail::GenerateRandomString();
        auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

        Traits::WaitHandleFactory waitHandleFactory;

        Server<int, void, Traits> server{
            std::make_unique<Server<int, void, Traits>::Connection>(
                closeEvent, closeEvent, closeEvent, waitHandleFactory,
                InputChannel<detail::ServerTraits<int, void, Traits>::InputPacket, Traits>{ create_only, name.c_str(), memory, waitHandleFactory }),
            [&](int x) { data = x; invoked = true; },
            [&] { closed = true; } };

        OutputChannel<detail::ServerTraits<int, void, Traits>::InputPacket, Traits>{ open_only, name.c_str(), memory }.Send(765);

        BOOST_TEST(waitHandleFactory.Process() != 0);

        BOOST_TEST(data == 765);
        BOOST_TEST(invoked);
    }
    BOOST_TEST(closed);
}

BOOST_AUTO_TEST_CASE(HandlerWithResponseInvocationTest)
{
    bool invoked = false, closed = false;
    {
        detail::KernelEvent closeEvent{ create_only, false };
        auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
        auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

        Traits::WaitHandleFactory waitHandleFactory;

        Server<int, int, Traits> server{
            std::make_unique<Server<int, int, Traits>::Connection>(
                closeEvent, closeEvent, closeEvent, waitHandleFactory,
                InputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
                OutputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
            [&](int x, auto&& callback) { callback(x * 2); invoked = true; },
            [&] { closed = true; } };

        OutputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
            .Send(detail::ServerTraits<int, int, Traits>::InputPacket{ Traits::PacketId{ 1000 }, 555 });

        BOOST_TEST(waitHandleFactory.Process() != 0);
        BOOST_TEST(invoked);

        BOOST_TEST(1 == (InputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ open_only, names.second.c_str(), memory, waitHandleFactory }
            .ReceiveAll([&](auto&& packet)
            {
                BOOST_TEST(packet.GetId() == Traits::PacketId{ 1000 });
                BOOST_TEST(packet.GetPayload() == 2 * 555);
            })));
    }
    BOOST_TEST(closed);
}

BOOST_AUTO_TEST_CASE(DestroyedWaitHandlesAfterDestructionTest)
{
    bool processed = false, closed = false;

    detail::KernelEvent closeEvent{ create_only, false };
    auto name = detail::GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    auto server = std::make_unique<Server<int, void, Traits>>(
        std::make_unique<Server<int, void, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, waitHandleFactory,
            InputChannel<detail::ServerTraits<int, void, Traits>::InputPacket, Traits>{ create_only, name.c_str(), memory, waitHandleFactory }),
        [&](int) { processed = true; },
        [&] { closed = true; });

    OutputChannel<detail::ServerTraits<int, void, Traits>::InputPacket, Traits>{ open_only, name.c_str(), memory }.Send(0);

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(processed);

    processed = false;
    server.reset();

    BOOST_TEST(closed);

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!processed);
}

BOOST_AUTO_TEST_CASE(DroppedResponseCallbackTest)
{
    bool invoked = false;

    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Server<int, int, Traits> server{
        std::make_unique<Server<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, waitHandleFactory,
            InputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
            OutputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [&](int, auto&&) { invoked = true; },
        [] {} };

    OutputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
        .Send(detail::ServerTraits<int, int, Traits>::InputPacket{ Traits::PacketId{ 10 }, 0 });

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(invoked);

    BOOST_TEST(1 == (InputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ open_only, names.second.c_str(), memory }
        .ReceiveAll([&](auto&& packet)
        {
            BOOST_TEST(packet.GetId() == Traits::PacketId{ 10 });
            BOOST_TEST(!packet.IsValid());
        })));
}

BOOST_AUTO_TEST_CASE(ClosedConnectionDroppedResponseCallbackTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    detail::Callback<void(int)> responseCallback;
    bool closed = false;

    Traits::WaitHandleFactory waitHandleFactory;

    auto server = std::make_unique<Server<int, int, Traits>>(
        std::make_unique<Server<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, waitHandleFactory,
            InputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
            OutputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [&](int, auto&& callback) { responseCallback = std::forward<decltype(callback)>(callback); },
        [&] { closed = true; });

    OutputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
        .Send(detail::ServerTraits<int, int, Traits>::InputPacket{ Traits::PacketId{ 1 }, 0 });

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(!!responseCallback);

    BOOST_TEST(closeEvent.Signal());

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(closed);

    responseCallback = {};

    InputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits> in{ open_only, names.second.c_str(), memory, waitHandleFactory };
    server.reset();

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(in.IsEmpty());
}

BOOST_AUTO_TEST_CASE(ClosedConnectionResponseCallbackInvocationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    detail::Callback<void(int)> responseCallback;
    bool closed = false;

    Traits::WaitHandleFactory waitHandleFactory;

    Server<int, int, Traits> server{
        std::make_unique<Server<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, waitHandleFactory,
            InputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
            OutputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [&](int, auto&& callback) { responseCallback = std::forward<decltype(callback)>(callback); },
        [&] { closed = true; } };

    OutputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
        .Send(detail::ServerTraits<int, int, Traits>::InputPacket{ Traits::PacketId{ 1 }, 0 });

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(!!responseCallback);

    BOOST_TEST(closeEvent.Signal());

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(closed);

    BOOST_CHECK_THROW(responseCallback(0), std::exception);
}

BOOST_AUTO_TEST_CASE(DestructedServerResponseCallbackInvocationTest)
{
    detail::Callback<void(int)> responseCallback;
    {
        detail::KernelEvent closeEvent{ create_only, false };
        auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
        auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

        Traits::WaitHandleFactory waitHandleFactory;

        Server<int, int, Traits> server{
            std::make_unique<Server<int, int, Traits>::Connection>(
                closeEvent, closeEvent, closeEvent, waitHandleFactory,
                InputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
                OutputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
            [&](int, auto&& callback) { responseCallback = std::forward<decltype(callback)>(callback); },
            [] {} };

        OutputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
            .Send(detail::ServerTraits<int, int, Traits>::InputPacket{ Traits::PacketId{ 1 }, 0 });

        BOOST_TEST(waitHandleFactory.Process() != 0);
    }
    BOOST_TEST(!!responseCallback);
    BOOST_CHECK_THROW(responseCallback(0), std::exception);
}

BOOST_AUTO_TEST_CASE(ConnectionClosedBeforeHandlerDestructionTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    auto trackLifetimeOrder = [index = 0](int& order) mutable
    {
        return std::shared_ptr<void>{ nullptr, [&](void*) { order = ++index; } };
    };

    int handlerOrder, closeHandlerOrder;

    Server<int, int, Traits>{
        std::make_unique<Server<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, Traits::WaitHandleFactory{},
            InputChannel<detail::ServerTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory },
            OutputChannel<detail::ServerTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [lifetime = trackLifetimeOrder(handlerOrder)](int, auto&&) {},
        [lifetime = trackLifetimeOrder(closeHandlerOrder)] {} };

    BOOST_TEST(handlerOrder == 2);
    BOOST_TEST(closeHandlerOrder == 1);
}

BOOST_AUTO_TEST_SUITE_END()
