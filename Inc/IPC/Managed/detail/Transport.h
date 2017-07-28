#pragma once

#include "Interop/Transport.h"
#include "NativeObject.h"
#include "Cast.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
#pragma warning(push)
#pragma warning(disable : 4564)
        template <typename Request, typename Response>
        [IPC::Managed::Object]
        ref class Transport : ITransport<Request, Response>
        {
            using IClient = Managed::IClient<Request, Response>;
            using IServer = Managed::IServer<Request, Response>;

            using IClientConnector = Managed::IClientConnector<Request, Response>;
            using IServerAcceptor = Managed::IServerAcceptor<Request, Response>;

            using IClientAccessor = Managed::IClientAccessor<Request, Response>;
            using IServersAccessor = Managed::IServersAccessor<Request, Response>;

            using Handler = Managed::Handler<Request, Response>;
            using HandlerFactory = Managed::HandlerFactory<Request, Response>;

            using NativeRequest = CastType<Request>;
            using NativeResponse = CastType<Response>;

            using NativeTransport = Interop::Transport<NativeRequest, NativeResponse>;
            using NativeConfig = std::shared_ptr<const Interop::Config>;

            using NativeClient = typename NativeTransport::Client;
            using NativeServer = typename NativeTransport::Server;

            using NativeClientConnector = typename NativeTransport::ClientConnector;
            using NativeServerAcceptor = typename NativeTransport::ServerAcceptor;

            using NativeClientAccessor = Interop::Callback<std::shared_ptr<void>()>;
            using NativeServersAccessor = Interop::Callback<Interop::Callback<const typename NativeTransport::ServerCollection&()>()>;

        public:
            explicit Transport(Config^ config);

            virtual IServerAcceptor^ MakeServerAcceptor(System::String^ name, HandlerFactory^ handlerFactory);

            virtual IClientConnector^ MakeClientConnector();

            virtual IClientAccessor^ ConnectClient(
                System::String^ acceptorName, System::Boolean async, System::TimeSpan timeout, IClientConnector^ connector, System::Boolean enabled);

            virtual IServersAccessor^ AcceptServers(System::String^ name, HandlerFactory^ handlerFactory, System::Boolean enabled);

        internal:
            ref class Server;

            ref class Client;

            ref class ServerAcceptor;

            ref class ClientConnector;

            ref class ClientAccessor;

            ref class ServersAccessor;

        private:
            NativeObject<NativeTransport> m_transport;
            NativeObject<NativeConfig> m_config;
            System::Lazy<IClientConnector^> m_clientConnector;
        };
#pragma warning(pop)

    } // detail

} // Managed
} // IPC
