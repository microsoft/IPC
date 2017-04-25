#pragma once
#pragma managed(push, off)

#include "IPC/ClientFwd.h"
#include "IPC/ServerFwd.h"
#include "IPC/AcceptorFwd.h"
#include "IPC/ConnectorFwd.h"
#include "Callback.h"
#include "SharedMemory.h"
#include "Policies/ReceiverFactory.h"
#include "Policies/TimeoutFactory.h"
#include "Policies/WaitHandleFactory.h"
#include "Policies/TransactionManagerFactory.h"
#include <memory>
#include <list>
#include <chrono>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        struct Traits : IPC::DefaultTraits
        {
            using WaitHandleFactory = Interop::Policies::WaitHandleFactory;

            using ReceiverFactory = Interop::Policies::ReceiverFactory;

            using TimeoutFactory = Interop::Policies::TimeoutFactory;

            template <typename Context>
            using TransactionManager = IPC::Policies::TransactionManager<Context, TimeoutFactory>;

            using TransactionManagerFactory = Interop::Policies::TransactionManagerFactory;
        };


        struct Config;


        template <typename Request, typename Response>
        class Transport
        {
        public:
            using CloseHandler = Callback<void()>;


            class Server : public std::unique_ptr<IPC::Server<Request, Response, Traits>>
            {
            public:
                struct ConnectionPtr : std::unique_ptr<IPC::ServerConnection<Request, Response, Traits>>
                {
                    using unique_ptr::unique_ptr;

                    ConnectionPtr(ConnectionPtr&& other);

                    ~ConnectionPtr();

                    SharedMemory GetInputMemory() const;

                    SharedMemory GetOutputMemory() const;
                };

                using Handler = Callback<void(Request&&, Callback<void(const Response&)>&&)>;

                Server(ConnectionPtr&& connection, CloseHandler&& closeHandler, Handler&& handler, const Config& config);

                ~Server();

                bool IsClosed() const;

                void Close();

                void RegisterCloseHandler(CloseHandler&& closeHandler);

                SharedMemory GetInputMemory() const;

                SharedMemory GetOutputMemory() const;
            };


            class Client : public std::unique_ptr<IPC::Client<Request, Response, Traits>>
            {
            public:
                struct ConnectionPtr : std::unique_ptr<IPC::ClientConnection<Request, Response, Traits>>
                {
                    using unique_ptr::unique_ptr;

                    ConnectionPtr(ConnectionPtr&& other);

                    ~ConnectionPtr();

                    SharedMemory GetInputMemory() const;

                    SharedMemory GetOutputMemory() const;
                };

                Client(ConnectionPtr&& connection, CloseHandler&& closeHandler, const Config& config);

                ~Client();

                void operator()(const Request& request, Callback<void(Response&&)>&& callback, const std::chrono::milliseconds& timeout);

                bool IsClosed() const;

                void Close();

                void RegisterCloseHandler(CloseHandler&& closeHandler);

                SharedMemory GetInputMemory() const;

                SharedMemory GetOutputMemory() const;
            };


            class ServerAcceptor : public std::unique_ptr<IPC::ServerAcceptor<Request, Response, Traits>>
            {
                using ConnectionPtr = typename Server::ConnectionPtr;

            public:
                using Handler = Callback<ConnectionPtr()>;
                using HandlerFactory = Callback<void(Handler&&)>;

                explicit ServerAcceptor(const char* name, HandlerFactory&& callback, const Config& config);

                ~ServerAcceptor();
            };


            class ClientConnector : public std::shared_ptr<IPC::ClientConnector<Request, Response, Traits>>
            {
                using ConnectionPtr = typename Client::ConnectionPtr;

            public:
                using Handler = Callback<ConnectionPtr()>;
                using HandlerFactory = Callback<void(Handler&&)>;

                explicit ClientConnector(const Config& config);

                ~ClientConnector();

                void Connect(const char* acceptorName, HandlerFactory&& handlerFactory, const std::chrono::milliseconds& timeout);
            };


            using ErrorHandler = Callback<void(std::exception_ptr)>;

            template <typename T>
            using ComponentFactory = Callback<std::shared_ptr<void>(typename T::ConnectionPtr&&, CloseHandler&&)>;

            using ServerCollection = std::list<std::shared_ptr<void>>;


            Transport(
                std::size_t outputMemorySize,
                const std::chrono::milliseconds& defaultRequestTimeout,
                Traits::ReceiverFactory receiverFactory,
                Traits::WaitHandleFactory waitHandleFactory,
                Traits::TimeoutFactory timeoutFactory);

            ~Transport();

            const std::shared_ptr<const Config>& GetConfig() const;

            Callback<std::shared_ptr<void>()> ConnectClient(
                const char* acceptorName,
                const ClientConnector& connector,
                bool async,
                ComponentFactory<Client>&& componentFactory,
                ErrorHandler&& errorHandler,
                const std::chrono::milliseconds& timeout);

            Callback<Callback<const ServerCollection&()>()> AcceptServers(
                const char* name,
                ComponentFactory<Server>&& componentFactory,
                ErrorHandler&& errorHandler);

        private:
            std::shared_ptr<const Config> m_config;
        };

    } // Interop
    } // detail

} // Managed
} // IPC

#pragma managed(pop)
