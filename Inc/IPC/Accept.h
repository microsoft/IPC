#pragma once

#include "detail/Accept.h"
#include "Acceptor.h"
#include "ComponentCollection.h"
#include "Client.h"
#include "Server.h"
#include <memory>


namespace IPC
{
    template <typename Request, typename Response, typename Traits = DefaultTraits, typename HandlerFactory, typename ErrorHandler = typename Traits::ErrorHandler>
    auto AcceptServers(
        const char* name,
        HandlerFactory&& handlerFactory,
        ChannelSettings<Traits> channelSettings = {},
        std::size_t hostInfoMemorySize = 0,
        ErrorHandler&& errorHandler = {})
    {
        using Server = Server<Request, Response, Traits>;

        return detail::Accept<ServerAcceptor<Request, Response, Traits>>(
            name,
            std::make_shared<ServerCollection<Server>>(),
            [handlerFactory = std::forward<HandlerFactory>(handlerFactory)](auto&& connection, auto&& closeHandler) mutable
            {
                auto handler = handlerFactory(*connection);

                return std::make_unique<Server>(
                    std::move(connection), std::forward<decltype(handler)>(handler), std::forward<decltype(closeHandler)>(closeHandler));
            },
            std::forward<ErrorHandler>(errorHandler),
            std::move(channelSettings),
            hostInfoMemorySize);
    }


    template <typename Request, typename Response, typename Traits = DefaultTraits, typename ErrorHandler = typename Traits::ErrorHandler>
    auto AcceptClients(
        const char* name,
        ChannelSettings<Traits> channelSettings = {},
        std::size_t hostInfoMemorySize = 0,
        ErrorHandler&& errorHandler = {},
        typename Traits::TransactionManagerFactory transactionManagerFactory = {})
    {
        using Client = Client<Request, Response, Traits>;

        return detail::Accept<ClientAcceptor<Request, Response, Traits>>(
            name,
            std::make_shared<ClientCollection<Client>>(),
            [transactionManagerFactory = std::move(transactionManagerFactory)](auto&& connection, auto&& closeHandler)
            {
                return std::make_unique<Client>(
                    std::move(connection),
                    std::forward<decltype(closeHandler)>(closeHandler),
                    transactionManagerFactory(detail::Identity<typename Client::TransactionManager>{}));
            },
            std::forward<ErrorHandler>(errorHandler),
            std::move(channelSettings),
            hostInfoMemorySize);
    }

} // IPC
