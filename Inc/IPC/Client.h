#pragma once

#include "ClientFwd.h"
#include "detail/PacketConnectionHolder.h"
#include "detail/Packet.h"
#include "detail/Callback.h"
#include <future>


namespace IPC
{
    namespace detail
    {
        template <typename Request, typename Response, typename Traits>
        class ClientTraits : public ClientPacketInfo<Request, Response, Traits>
        {
            struct NullTransactionManager
            {};

        public:
            using Context = std::conditional_t<
                std::is_void<Response>::value,
                NullTransactionManager,
                typename Traits::template TransactionManager<Callback<void(Response)>>>;
        };

    } // detail


    template <typename Request, typename Response, typename Traits>
    class Client : public detail::PacketConnectionHolder<detail::ClientTraits<Request, Response, Traits>>
    {
        using PacketConnectionHolder = detail::PacketConnectionHolder<detail::ClientTraits<Request, Response, Traits>>;

    protected:
        using typename PacketConnectionHolder::InputPacket;
        using typename PacketConnectionHolder::OutputPacket;

    public:
        static_assert(!std::is_void<Request>::value, "Request cannot be void.");
        static_assert(std::is_same<Connection, ClientConnection<Request, Response, Traits>>::value, "Connection definitions must be the same.");

        using typename PacketConnectionHolder::Connection;
        using TransactionManager = typename PacketConnectionHolder::Context;

        template <typename CloseHandler, typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        Client(std::unique_ptr<Connection> connection, CloseHandler&& closeHandler, TransactionManager transactionManager = {})
            : PacketConnectionHolder{
                std::move(connection),
                std::move(transactionManager),
                [this](InputPacket&& packet) { ResponseHandler(std::move(packet)); },
                [this, closeHandler = std::forward<CloseHandler>(closeHandler)]() mutable
                {
                    GetTransactionManager().TerminateTransactions();
                    closeHandler();
                } }
        {}

        template <typename CloseHandler, typename U = Response, std::enable_if_t<std::is_void<U>::value>* = nullptr>
        Client(std::unique_ptr<Connection> connection, CloseHandler&& closeHandler)
            : PacketConnectionHolder{ std::move(connection), {}, std::forward<CloseHandler>(closeHandler) }
        {}

        template <typename OtherRequest, typename Callback, typename... TransactionArgs, typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr,
            decltype(std::declval<Callback>()(std::declval<U>()))* = nullptr>
        void operator()(OtherRequest&& request, Callback&& callback, TransactionArgs&&... transactionArgs)
        {
            auto&& id = GetTransactionManager().BeginTransaction(std::forward<Callback>(callback), std::forward<TransactionArgs>(transactionArgs)...);

            try
            {
                SendRequest(std::forward<OtherRequest>(request), id);
            }
            catch (...)
            {
                GetTransactionManager().EndTransaction(id);
                throw;
            }
        }

        template <typename OtherRequest, typename U = Response, std::enable_if_t<std::is_void<U>::value>* = nullptr>
        void operator()(OtherRequest&& request)
        {
            SendRequest(std::forward<OtherRequest>(request));
        }

        template <typename OtherRequest, typename... TransactionArgs, typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        std::future<Response> operator()(OtherRequest&& request, TransactionArgs&&... transactionArgs)
        {
            std::packaged_task<Response(Response&&)> callback{ [](Response&& response) { return response; } };

            auto result = callback.get_future();

            operator()(std::forward<OtherRequest>(request), std::move(callback), std::forward<TransactionArgs>(transactionArgs)...);

            return result;
        }

    private:
        auto& GetTransactionManager()
        {
            return this->GetContext();
        }

        template <typename OtherRequest, typename... Args>
        void SendRequest(OtherRequest&& request, Args&&... args)
        {
            this->GetConnection().GetOutputChannel().Send(OutputPacket{ std::forward<Args>(args)..., std::forward<OtherRequest>(request) });
        }

        template <typename Packet = InputPacket, typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        void ResponseHandler(Packet&& packet)
        {
            if (auto callback = GetTransactionManager().EndTransaction(packet.GetId()))
            {
                if (packet.IsValid())
                {
                    (*callback)(std::move(packet.GetPayload()));
                }
            }
        }
    };

} // IPC
