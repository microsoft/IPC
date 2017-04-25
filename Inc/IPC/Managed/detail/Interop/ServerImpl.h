#pragma once

#include "Transport.h"
#include "IPC/Server.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename Request, typename Response>
        Transport<Request, Response>::Server::ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) = default;

        template <typename Request, typename Response>
        Transport<Request, Response>::Server::ConnectionPtr::~ConnectionPtr() = default;

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Server::ConnectionPtr::GetInputMemory() const
        {
            return this->get()->GetInputChannel().GetMemory();
        }

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Server::ConnectionPtr::GetOutputMemory() const
        {
            return this->get()->GetOutputChannel().GetMemory();
        }


        template <typename Request, typename Response>
        Transport<Request, Response>::Server::Server(ConnectionPtr&& connection, CloseHandler&& closeHandler, Handler&& handler, const Config& /*config*/)
            : unique_ptr{ std::make_unique<IPC::Server<Request, Response, Traits>>(std::move(connection), std::move(handler), std::move(closeHandler)) }
        {}

        template <typename Request, typename Response>
        Transport<Request, Response>::Server::~Server() = default;

        template <typename Request, typename Response>
        bool Transport<Request, Response>::Server::IsClosed() const
        {
            return this->get()->GetConnection().IsClosed();
        }

        template <typename Request, typename Response>
        void Transport<Request, Response>::Server::Close()
        {
            this->get()->GetConnection().Close();
        }

        template <typename Request, typename Response>
        void Transport<Request, Response>::Server::RegisterCloseHandler(CloseHandler&& closeHandler)
        {
            this->get()->GetConnection().RegisterCloseHandler(std::move(closeHandler), true);
        }

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Server::GetInputMemory() const
        {
            return this->get()->GetConnection().GetInputChannel().GetMemory();
        }

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Server::GetOutputMemory() const
        {
            return this->get()->GetConnection().GetOutputChannel().GetMemory();
        }

    } // Interop
    } // detail

} // Managed
} // IPC
