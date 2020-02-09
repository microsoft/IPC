#pragma once

#include "Transport.h"
#include "IPC/Connector.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename Request, typename Response>
        Transport<Request, Response>::ClientConnector::ClientConnector(const Config& config)
            : ClientConnector::shared_ptr{ std::make_shared<IPC::ClientConnector<Request, Response, Traits>>(
                config.m_channelSettings,
                Policies::TransactionManagerFactory{ config.m_timeoutFactory, config.m_defaultRequestTimeout }) }
        {}

        template <typename Request, typename Response>
        Transport<Request, Response>::ClientConnector::~ClientConnector() = default;

        template <typename Request, typename Response>
        void Transport<Request, Response>::ClientConnector::Connect(const char* acceptorName, HandlerFactory&& handlerFactory, const std::chrono::milliseconds& timeout)
        {
            this->get()->Connect(
                acceptorName,
                [handlerFactory = std::move(handlerFactory)](auto&& futureConnection) mutable
                {
                    handlerFactory([futureConnection = std::move(futureConnection)]() mutable { return futureConnection.get(); });
                },
                timeout);
        }

    } // Interop
    } // detail

} // Managed
} // IPC
