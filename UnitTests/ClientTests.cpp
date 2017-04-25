#include "stdafx.h"
#include "IPC/Client.h"
#include "IPC/detail/RandomString.h"
#include "TraitsMock.h"
#include <type_traits>
#include <memory>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ClientTests)

constexpr std::size_t c_queueLimit = 20;
constexpr std::size_t c_memSize = 10240;

template <typename Context>
class TransactionManagerMock : public std::shared_ptr<std::vector<std::shared_ptr<void>>>
{
public:
    using Id = std::uint32_t;

    TransactionManagerMock()
        : shared_ptr{ std::make_shared<std::vector<std::shared_ptr<void>>>() }
    {}

    template <typename OtherContext, typename... Args>
    Id BeginTransaction(OtherContext&& /*context*/, Args&&... args)
    {
        this->get()->push_back(std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...));
        return 0;
    }

    boost::optional<Context> EndTransaction(const Id& /*id*/)
    {
        return boost::none;
    }

    void TerminateTransactions()
    {}
};

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

struct TransactionManagerMockTraits : Traits
{
    template <typename Context>
    using TransactionManager = TransactionManagerMock<Context>;
};

static_assert(!std::is_copy_constructible<Client<int, int>>::value, "Client should not be copy constructible.");
static_assert(!std::is_copy_assignable<Client<int, int>>::value, "Client should not be copy assignable.");
static_assert(!std::is_move_constructible<Client<int, int>>::value, "Client should not be move constructible.");
static_assert(!std::is_move_assignable<Client<int, int>>::value, "Client should not be move assignable.");

BOOST_AUTO_TEST_CASE(NoResponseInvocationTest)
{
    bool closed = false;
    {
        detail::KernelEvent closeEvent{ create_only, false };
        auto name = detail::GenerateRandomString();
        auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), c_memSize);

        Client<int, void, Traits> client{
            std::make_unique<Client<int, void, Traits>::Connection>(
                closeEvent, closeEvent, closeEvent, Traits::WaitHandleFactory{},
                OutputChannel<detail::ClientTraits<int, void, Traits>::OutputPacket, Traits>{ create_only, name.c_str(), memory }),
            [&] { closed = true; } };

        BOOST_CHECK_NO_THROW(client(444));

        BOOST_TEST(1 == (InputChannel<detail::ClientTraits<int, void, Traits>::OutputPacket, Traits>{ open_only, name.c_str(), memory }
            .ReceiveAll([&](auto&& packet)
            {
                BOOST_TEST(packet.GetPayload() == 444);
            })));
    }
    BOOST_TEST(closed);
}

BOOST_AUTO_TEST_CASE(HandlerWithResponseInvocationTest)
{
    bool closed = false;
    {
        detail::KernelEvent closeEvent{ create_only, false };
        auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
        auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

        Traits::WaitHandleFactory waitHandleFactory;

        Client<int, int, Traits> client{
            std::make_unique<Client<int, int, Traits>::Connection>(
                closeEvent, closeEvent, closeEvent, waitHandleFactory,
                InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
                OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
            [&] { closed = true; } };

        auto result = client(333);
        BOOST_TEST(result.valid());
        BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));

        Traits::PacketId id;
        BOOST_TEST(1 == (InputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ open_only, names.second.c_str(), memory, waitHandleFactory }
            .ReceiveAll([&](auto&& packet)
            {
                id = packet.GetId();
                BOOST_TEST(packet.GetPayload() == 333);
            })));

        OutputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
            .Send(detail::ClientTraits<int, int, Traits>::InputPacket{ id, 111 });

        BOOST_TEST(waitHandleFactory.Process() != 0);
        BOOST_TEST(result.get() == 111);
    }
    BOOST_TEST(closed);
}

BOOST_AUTO_TEST_CASE(DestroyedWaitHandlesAfterDestructionTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    bool received = false, closed = false;

    auto client = std::make_unique<Client<int, int, Traits>>(
        std::make_unique<Client<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, waitHandleFactory,
            InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
            OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [&] { closed = true; });

    (*client)(0, [&](int) { received = true; });

    BOOST_TEST(1 == (InputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ open_only, names.second.c_str(), memory, waitHandleFactory }
        .ReceiveAll([&](auto&& packet)
        {
            OutputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ open_only, names.first.c_str(), memory }
                .Send(detail::ClientTraits<int, int, Traits>::InputPacket{ packet.GetId(), 0 });
        })));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(received);

    received = false;
    client.reset();

    BOOST_TEST(closed);

    BOOST_TEST(waitHandleFactory.Process() == 0);
    BOOST_TEST(waitHandleFactory->empty());
    BOOST_TEST(!received);
}

BOOST_AUTO_TEST_CASE(ClosedConnectionInvocationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    Client<int, int, Traits> client{
        std::make_unique<Client<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, Traits::WaitHandleFactory{},
            InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory },
            OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [] {} };

    client.GetConnection().Close();

    BOOST_CHECK_THROW(client(1), std::exception);

    auto context = std::make_shared<int>();
    bool invoked = false;
    BOOST_CHECK_THROW((client(2, [&, context](int) { invoked = true; })), std::exception);
    BOOST_TEST(!invoked);
    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(ClosedConnectionTransactionTerminationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    bool closed = false;

    Client<int, int, Traits> client{
        std::make_unique<Client<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, Traits::WaitHandleFactory{},
            InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory },
            OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [&] { closed = true; } };

    auto result = client(1);
    BOOST_TEST(result.valid());
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));

    auto context = std::make_shared<int>();
    bool invoked = false;
    client(2, [&, context](int) { invoked = true; });
    BOOST_TEST(context.use_count() == 2);

    client.GetConnection().Close();

    BOOST_TEST(closed);

    BOOST_CHECK_THROW(result.get(), std::future_error);
    BOOST_TEST(!invoked);
    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(TransactionTerminationDuringDestructionTest)
{
    bool invoked = false;
    std::future<int> result;
    auto context = std::make_shared<int>();
    {
        detail::KernelEvent closeEvent{ create_only, false };
        auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
        auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

        Client<int, int, Traits> client{
            std::make_unique<Client<int, int, Traits>::Connection>(
                closeEvent, closeEvent, closeEvent, Traits::WaitHandleFactory{},
                InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory },
                OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
            [] {} };

        result = client(1);
        BOOST_TEST(result.valid());
        BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));

        client(2, [&, context](int) { invoked = true; });
        BOOST_TEST(context.use_count() == 2);
    }
    BOOST_CHECK_THROW(result.get(), std::future_error);
    BOOST_TEST(!invoked);
    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(InvalidPacketTransactionTerminationTest)
{
    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    Traits::WaitHandleFactory waitHandleFactory;

    Client<int, int, Traits> client{
        std::make_unique<Client<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, waitHandleFactory,
            InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory, waitHandleFactory },
            OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [] {} };

    auto result = client(1);
    BOOST_TEST(result.valid());
    BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));

    auto context = std::make_shared<int>();
    bool invoked = false;
    client(2, [&, context](int) { invoked = true; });
    BOOST_TEST(context.use_count() == 2);

    OutputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits> out{ open_only, names.first.c_str(), memory };

    BOOST_TEST(2 == (InputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ open_only, names.second.c_str(), memory, waitHandleFactory }
        .ReceiveAll([&](auto&& packet) { out.Send(packet.GetId()); })));

    BOOST_TEST(waitHandleFactory.Process() != 0);
    BOOST_TEST(out.IsEmpty());

    BOOST_CHECK_THROW(result.get(), std::future_error);
    BOOST_TEST(!invoked);
    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(CustomTransactionArgsTest)
{
    using Traits = TransactionManagerMockTraits;

    detail::KernelEvent closeEvent{ create_only, false };
    auto names = std::make_pair(detail::GenerateRandomString(), detail::GenerateRandomString());
    auto memory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_memSize);

    const char* text = "Random text";

    Client<int, int, Traits>::TransactionManager transactionManager;

    Client<int, int, Traits> client{
        std::make_unique<Client<int, int, Traits>::Connection>(
            closeEvent, closeEvent, closeEvent, Traits::WaitHandleFactory{},
            InputChannel<detail::ClientTraits<int, int, Traits>::InputPacket, Traits>{ create_only, names.first.c_str(), memory },
            OutputChannel<detail::ClientTraits<int, int, Traits>::OutputPacket, Traits>{ create_only, names.second.c_str(), memory }),
        [] {},
        transactionManager };

    client(1, [](auto&&) {}, text, 3);
    client(2, 3, text);

    BOOST_TEST(transactionManager->size() == 2);
    {
        const auto& args = std::static_pointer_cast<std::tuple<const char*, int>>(transactionManager->front());
        BOOST_TEST(args);
        BOOST_TEST((*args == std::make_tuple(text, 3)));
    }
    {
        const auto& args = std::static_pointer_cast<std::tuple<int, const char*>>(transactionManager->back());
        BOOST_TEST(args);
        BOOST_TEST((*args == std::make_tuple(3, text)));
    }
}

BOOST_AUTO_TEST_SUITE_END()
