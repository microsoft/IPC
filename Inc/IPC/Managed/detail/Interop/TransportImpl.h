#pragma once

#include "IPC/Connect.h"
#include "IPC/Accept.h"
#include "Transport.h"
#include "ClientImpl.h"
#include "ServerImpl.h"
#include "ClientConnectorImpl.h"
#include "ServerAcceptorImpl.h"
#include <chrono>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        struct Config
        {
            Config(
                ChannelSettings<Traits> channelSettings,
                const std::chrono::milliseconds& defaultRequestTimeout,
                Traits::TimeoutFactory timeoutFactory)
                : m_channelSettings{ std::move(channelSettings) },
                  m_defaultRequestTimeout{ defaultRequestTimeout },
                  m_timeoutFactory{ std::move(timeoutFactory) }
            {}

            ChannelSettings<Traits> m_channelSettings;
            std::chrono::milliseconds m_defaultRequestTimeout;
            Traits::TimeoutFactory m_timeoutFactory;
        };


        auto MakeChannelSettings(std::size_t memorySize, Traits::ReceiverFactory receiverFactory, Traits::WaitHandleFactory waitHandleFactory)
        {
            ChannelSettings<Traits> settings{ std::move(waitHandleFactory), std::move(receiverFactory) };

            if (memorySize != 0)
            {
                settings.SetInput(memorySize, true);
                settings.SetOutput(memorySize, true);
            }

            return settings;
        }


        template <typename Request, typename Response>
        Transport<Request, Response>::Transport(
            std::size_t outputMemorySize,
            const std::chrono::milliseconds& defaultRequestTimeout,
            Traits::ReceiverFactory receiverFactory,
            Traits::WaitHandleFactory waitHandleFactory,
            Traits::TimeoutFactory timeoutFactory)
            : m_config{ std::make_shared<Config>(
                MakeChannelSettings(outputMemorySize, std::move(receiverFactory), std::move(waitHandleFactory)),
                defaultRequestTimeout,
                std::move(timeoutFactory)) }
        {}


        template <typename Request, typename Response>
        Transport<Request, Response>::~Transport() = default;


        template <typename Request, typename Response>
        const std::shared_ptr<const Config>& Transport<Request, Response>::GetConfig() const
        {
            return m_config;
        }


        template <typename Request, typename Response>
        Callback<std::shared_ptr<void>()> Transport<Request, Response>::ConnectClient(
            const char* acceptorName,
            const ClientConnector& connector,
            bool async,
            ComponentFactory<Client>&& componentFactory,
            ErrorHandler&& errorHandler,
            const std::chrono::milliseconds& timeout)
        {
            return IPC::detail::Connect(
                acceptorName,
                connector,
                async,
                m_config->m_timeoutFactory,
                std::move(errorHandler),
                std::move(componentFactory),
                timeout);
        }


        template <typename Request, typename Response>
        auto Transport<Request, Response>::AcceptServers(
            const char* name,
            ComponentFactory<Server>&& componentFactory,
            ErrorHandler&& errorHandler)
            -> Callback<Callback<const ServerCollection&()>()>
        {
            auto accessor = IPC::detail::Accept<IPC::ServerAcceptor<Request, Response, Traits>>(
                name,
                std::make_shared<IPC::ComponentCollection<ServerCollection>>(),
                std::move(componentFactory),
                std::move(errorHandler),
                m_config->m_channelSettings);

            return [accessor = std::move(accessor)]() mutable
            {
                return [servers = accessor()]() -> const ServerCollection& { return *servers; };
            };
        }

    } // Interop
    } // detail

} // Managed
} // IPC
