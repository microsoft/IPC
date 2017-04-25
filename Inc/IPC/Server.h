#pragma once

#include "ServerFwd.h"
#include "detail/PacketConnectionHolder.h"
#include "detail/Packet.h"
#include "detail/Callback.h"
#include <memory>


namespace IPC
{
    namespace detail
    {
        template <typename Request, typename Response, typename Traits>
        class ServerTraits : public ServerPacketInfo<Request, Response, Traits>
        {
        public:
            using HandlerCallback = std::conditional_t<
                std::is_void<Response>::value,
                Callback<void(Request&&)>,
                Callback<void(Request&&, Callback<void(Response)>)>>;

            using Context = HandlerCallback;
        };

    } // detail


    template <typename Request, typename Response, typename Traits>
    class Server : public detail::PacketConnectionHolder<detail::ServerTraits<Request, Response, Traits>>
    {
        using PacketConnectionHolder = detail::PacketConnectionHolder<detail::ServerTraits<Request, Response, Traits>>;

    protected:
        using typename PacketConnectionHolder::InputPacket;
        using typename PacketConnectionHolder::OutputPacket;

    public:
        static_assert(!std::is_void<Request>::value, "Request cannot be void.");
        static_assert(std::is_same<Connection, ServerConnection<Request, Response, Traits>>::value, "Connection definitions must be the same.");

        using typename PacketConnectionHolder::Connection;
        using HandlerCallback = typename PacketConnectionHolder::HandlerCallback;

        template <typename Handler, typename CloseHandler>
        Server(std::unique_ptr<Connection> connection, Handler&& handler, CloseHandler&& closeHandler)
            : PacketConnectionHolder{
                std::move(connection),
                std::forward<Handler>(handler),
                [this](InputPacket&& packet) { RequestHandler(std::move(packet)); },
                std::forward<CloseHandler>(closeHandler) }
        {}

    private:
        template <typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        void RequestHandler(InputPacket&& packet)
        {
            struct State : std::pair<std::shared_ptr<Connection>, typename Traits::PacketId>
            {
                using std::pair<std::shared_ptr<Connection>, typename Traits::PacketId>::pair;

                State(State&& other) = default;

                ~State()
                {
                    if (auto& connection = this->first)
                    {
                        if (auto channel = connection->GetOutputChannel(std::nothrow))
                        {
                            channel->TrySend(OutputPacket{ this->second });
                        }
                    }
                }
            };

            auto id = packet.GetId();

            InvokeHandler(
                std::move(packet),
                [state = State{ this->GetSharedConnection(), id }](auto&& response) mutable
                {
                    auto connection = std::move(state.first);
                    assert(connection);
                    connection->GetOutputChannel().Send(OutputPacket{ state.second, std::forward<decltype(response)>(response) });
                });
        }

        template <typename U = Response, std::enable_if_t<std::is_void<U>::value>* = nullptr>
        void RequestHandler(InputPacket&& packet)
        {
            InvokeHandler(std::move(packet));
        }

        auto& GetHandler()
        {
            return this->GetContext();
        }

        template <typename... Args>
        void InvokeHandler(InputPacket&& packet, Args&&... args)
        {
            GetHandler()(std::move(packet.GetPayload()), std::forward<Args>(args)...);
        }
    };

} // IPC
