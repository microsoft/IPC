#pragma once

#include "Transport.h"
#include "IPC/Acceptor.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename Request, typename Response>
        Transport<Request, Response>::ServerAcceptor::ServerAcceptor(const char* name, HandlerFactory&& handlerFactory, const Config& config)
            : unique_ptr{ std::make_unique<IPC::ServerAcceptor<Request, Response, Traits>>(
                name,
                [handlerFactory = std::move(handlerFactory)](auto&& futureConnection) mutable
                {
                    handlerFactory([futureConnection = std::move(futureConnection)]() mutable { return futureConnection.get(); });
                },
                config.m_channelSettings) }
        {}

        template <typename Request, typename Response>
        Transport<Request, Response>::ServerAcceptor::~ServerAcceptor() = default;

    } // Interop
    } // detail

} // Managed
} // IPC
