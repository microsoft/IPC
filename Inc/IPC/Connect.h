#pragma once

#include "detail/Connect.h"
#include "DefaultTraits.h"
#include "Client.h"
#include "Server.h"
#include <memory>
#include <chrono>


namespace IPC
{
    template <
        typename PacketConnector,
        typename TimeoutFactory = typename PacketConnector::Traits::TimeoutFactory,
        typename ErrorHandler = typename PacketConnector::Traits::ErrorHandler,
        typename... TransactionArgs>
    auto ConnectClient(
        const char* acceptorName,
        std::shared_ptr<PacketConnector> connector,
        bool async,
        TimeoutFactory&& timeoutFactory = {},
        ErrorHandler&& errorHandler = {},
        typename PacketConnector::Traits::TransactionManagerFactory transactionManagerFactory = {},
        TransactionArgs&&... transactionArgs)
    {
        return detail::Connect(
            acceptorName,
            std::move(connector),
            async,
            std::forward<TimeoutFactory>(timeoutFactory),
            std::forward<ErrorHandler>(errorHandler),
            [transactionManagerFactory = std::move(transactionManagerFactory)](auto&& connection, auto&& closeHandler) mutable
            {
                using Client = Client<typename PacketConnector::Request, typename PacketConnector::Response, typename PacketConnector::Traits>;

                return std::make_unique<Client>(
                    std::move(connection),
                    std::forward<decltype(closeHandler)>(closeHandler),
                    transactionManagerFactory(detail::Identity<typename Client::TransactionManager>{}));
            },
            std::forward<TransactionArgs>(transactionArgs)...);
    }


    template <
        typename PacketConnector,
        typename HandlerFactory,
        typename TimeoutFactory = typename PacketConnector::Traits::TimeoutFactory,
        typename ErrorHandler = typename PacketConnector::Traits::ErrorHandler,
        typename... TransactionArgs>
    auto ConnectServer(
        const char* acceptorName,
        std::shared_ptr<PacketConnector> connector,
        HandlerFactory&& handlerFactory,
        bool async,
        TimeoutFactory&& timeoutFactory = {},
        ErrorHandler&& errorHandler = {},
        TransactionArgs&&... transactionArgs)
    {
        return detail::Connect(
            acceptorName,
            std::move(connector),
            async,
            std::forward<TimeoutFactory>(timeoutFactory),
            std::forward<ErrorHandler>(errorHandler),
            [handlerFactory = std::forward<HandlerFactory>(handlerFactory)](auto&& connection, auto&& closeHandler) mutable
            {
                using Server = Server<typename PacketConnector::Request, typename PacketConnector::Response, typename PacketConnector::Traits>;

                auto handler = handlerFactory(*connection);

                return std::make_unique<Server>(std::move(connection), std::move(handler), std::forward<decltype(closeHandler)>(closeHandler));
            },
            std::forward<TransactionArgs>(transactionArgs)...);
    }

} // IPC
