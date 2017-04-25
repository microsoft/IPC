#pragma once

#include "Acceptor.h"
#include "Accept.h"
#include "Connector.h"
#include "Connect.h"
#include <memory>
#include <mutex>


namespace IPC
{
    struct DefaultTraits;


    template <typename Request, typename Response, typename Traits = DefaultTraits>
    class Transport
    {
    public:
        using Client = Client<Request, Response, Traits>;
        using Server = Server<Request, Response, Traits>;
        using ClientConnector = ClientConnector<Request, Response, Traits>;
        using ServerConnector = ServerConnector<Request, Response, Traits>;
        using ClientAcceptor = ClientAcceptor<Request, Response, Traits>;
        using ServerAcceptor = ServerAcceptor<Request, Response, Traits>;

        Transport()
            : Transport{ {} }
        {}

        explicit Transport(
            ChannelSettings<Traits> channelSettings,
            std::size_t hostInfoMemorySize = 0,
            typename Traits::TimeoutFactory timeoutFactory = {},
            typename Traits::ErrorHandler errorHandler = {},
            typename Traits::TransactionManagerFactory transactionManagerFactory = {})
            : m_channelSettings{ std::move(channelSettings) },
              m_hostInfoMemorySize{ hostInfoMemorySize },
              m_timeoutFactory{ std::move(timeoutFactory) },
              m_errorHandler{ std::move(errorHandler) },
              m_transactionManagerFactory{ std::move(transactionManagerFactory) }
        {}

        auto MakeClientConnector()
        {
            return ClientConnector{ m_channelSettings, m_transactionManagerFactory };
        }

        auto MakeServerConnector()
        {
            return ServerConnector{ m_channelSettings, m_transactionManagerFactory };
        }

        template <typename Handler>
        auto MakeServerAcceptor(const char* name, Handler&& handler)
        {
            return ServerAcceptor{ name, std::forward<Handler>(handler), m_channelSettings, m_hostInfoMemorySize };
        }

        template <typename Handler>
        auto MakeClientAcceptor(const char* name, Handler&& handler)
        {
            return ClientAcceptor{ name, std::forward<Handler>(handler), m_channelSettings, m_hostInfoMemorySize };
        }

        template <typename... TransactionArgs>
        auto ConnectClient(
            const char* name,
            bool async,
            std::shared_ptr<ClientConnector> connector = {},
            TransactionArgs&&... transactionArgs)
        {
            if (!connector)
            {
                std::call_once(
                    m_clientConnectorOnceFlag,
                    [this] { m_clientConnector = std::make_shared<ClientConnector>(MakeClientConnector()); });

                connector = m_clientConnector;
            }

            return IPC::ConnectClient(
                name,
                connector,
                async,
                m_timeoutFactory,
                m_errorHandler,
                m_transactionManagerFactory,
                std::forward<TransactionArgs>(transactionArgs)...);
        }

        template <typename HandlerFactory, typename... TransactionArgs>
        auto ConnectServer(
            const char* name,
            HandlerFactory&& handlerFactory,
            bool async,
            std::shared_ptr<ServerConnector> connector = {},
            TransactionArgs&&... transactionArgs)
        {
            if (!connector)
            {
                std::call_once(
                    m_serverConnectorOnceFlag,
                    [this] { m_serverConnector = std::make_shared<ServerConnector>(MakeServerConnector()); });

                connector = m_serverConnector;
            }

            return IPC::ConnectServer(
                name,
                connector,
                std::forward<HandlerFactory>(handlerFactory),
                async,
                m_timeoutFactory,
                m_errorHandler,
                std::forward<TransactionArgs>(transactionArgs)...);
        }

        template <typename HandlerFactory>
        auto AcceptServers(const char* name, HandlerFactory&& handlerFactory)
        {
            return IPC::AcceptServers<Request, Response, Traits>(
                name, std::forward<HandlerFactory>(handlerFactory), m_channelSettings, m_hostInfoMemorySize, m_errorHandler);
        }

        auto AcceptClients(const char* name)
        {
            return IPC::AcceptClients<Request, Response, Traits>(
                name, m_channelSettings, m_hostInfoMemorySize, m_errorHandler, m_transactionManagerFactory);
        }

    private:
        ChannelSettings<Traits> m_channelSettings;
        std::size_t m_hostInfoMemorySize;
        typename Traits::TimeoutFactory m_timeoutFactory;
        typename Traits::ErrorHandler m_errorHandler;
        typename Traits::TransactionManagerFactory m_transactionManagerFactory;
        std::shared_ptr<ClientConnector> m_clientConnector;
        std::shared_ptr<ServerConnector> m_serverConnector;
        std::once_flag m_clientConnectorOnceFlag;
        std::once_flag m_serverConnectorOnceFlag;
    };

} // IPC
