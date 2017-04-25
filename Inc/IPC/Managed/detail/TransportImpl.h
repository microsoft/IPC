#pragma once

#pragma managed(push, off)
#include "IPC/DefaultTraitsFwd.h"
#pragma managed(pop)

#include "Transport.h"
#include "Client.h"
#include "Server.h"
#include "ClientConnector.h"
#include "ServerAcceptor.h"
#include "Connect.h"
#include "Accept.h"

#include "Policies/ReceiverFactory.h"
#include "Policies/WaitHandleFactory.h"
#include "Policies/TimeoutFactory.h"

#include <msclr/gcroot.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename Request, typename Response>
        Transport<Request, Response>::Transport(Config^ config)
            : m_transport{
                config->OutputMemorySize,
                std::chrono::milliseconds{ static_cast<std::chrono::milliseconds::rep>(config->DefaultRequestTimeout.TotalMilliseconds) },
                Policies::MakeReceiverFactory(),
                Policies::MakeWaitHandleFactory(),
                Policies::MakeTimeoutFactory(std::chrono::milliseconds{ static_cast<std::chrono::milliseconds::rep>(config->ReconnectTimeout.TotalMilliseconds) }) },
              m_config{ m_transport->GetConfig() },
              m_clientConnector{
                gcnew System::Func<IClientConnector^>(this, &Transport<Request, Response>::MakeClientConnector),
                System::Threading::LazyThreadSafetyMode::PublicationOnly }
        {}

        template <typename Request, typename Response>
        auto Transport<Request, Response>::MakeServerAcceptor(System::String^ name, HandlerFactory^ handlerFactory)
            -> IServerAcceptor^
        {
            return gcnew ServerAcceptor{ name, handlerFactory, *m_config };
        }

        template <typename Request, typename Response>
        auto Transport<Request, Response>::MakeClientConnector()
            -> IClientConnector^
        {
            return gcnew ClientConnector{ *m_config };
        }

    } // detail

} // Managed
} // IPC
