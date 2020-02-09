#pragma once

#include "Transport.h"
#include "IPC/Client.h"
#include <chrono>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename Request, typename Response>
        Transport<Request, Response>::Client::ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) = default;

        template <typename Request, typename Response>
        Transport<Request, Response>::Client::ConnectionPtr::~ConnectionPtr() = default;

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Client::ConnectionPtr::GetInputMemory() const
        {
            return this->get()->GetInputChannel().GetMemory();
        }

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Client::ConnectionPtr::GetOutputMemory() const
        {
            return this->get()->GetOutputChannel().GetMemory();
        }


        template <typename Request, typename Response>
        Transport<Request, Response>::Client::Client(ConnectionPtr&& connection, CloseHandler&& closeHandler, const Config& config)
            : Client::unique_ptr{ std::make_unique<IPC::Client<Request, Response, Traits>>(
                std::move(connection),
                std::move(closeHandler),
                typename IPC::Client<Request, Response, Traits>::TransactionManager{ config.m_timeoutFactory, config.m_defaultRequestTimeout }) }
        {}

        template <typename Request, typename Response>
        Transport<Request, Response>::Client::~Client() = default;

        template <typename Request, typename Response>
        void Transport<Request, Response>::Client::operator()(const Request& request, Callback<void(Response&&)>&& callback, const std::chrono::milliseconds& timeout)
        {
            this->get()->operator()(request, std::move(callback), timeout);
        }

        template <typename Request, typename Response>
        bool Transport<Request, Response>::Client::IsClosed() const
        {
            return this->get()->GetConnection().IsClosed();
        }

        template <typename Request, typename Response>
        void Transport<Request, Response>::Client::Close()
        {
            this->get()->GetConnection().Close();
        }

        template <typename Request, typename Response>
        void Transport<Request, Response>::Client::RegisterCloseHandler(CloseHandler&& closeHandler)
        {
            this->get()->GetConnection().RegisterCloseHandler(std::move(closeHandler), true);
        }

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Client::GetInputMemory() const
        {
            return this->get()->GetConnection().GetInputChannel().GetMemory();
        }

        template <typename Request, typename Response>
        SharedMemory Transport<Request, Response>::Client::GetOutputMemory() const
        {
            return this->get()->GetConnection().GetOutputChannel().GetMemory();
        }

    } // Interop
    } // detail

} // Managed
} // IPC
