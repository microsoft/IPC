#pragma once

#include "AcceptorFwd.h"
#include "Server.h"
#include "DefaultTraits.h"
#include "ComponentCollection.h"
#include "Exception.h"
#include "detail/ChannelFactory.h"
#include "detail/Info.h"
#include "detail/KernelEvent.h"
#include "detail/KernelProcess.h"
#include "detail/RandomString.h"
#include "detail/Apply.h"
#include <memory>
#include <future>


namespace IPC
{
    template <typename Input, typename Output, typename Traits>
    class Acceptor
    {
        using ChannelFactory = detail::ChannelFactory<Traits>;

    public:
        using Connection = Connection<Input, Output, Traits>;

        template <typename Handler>
        Acceptor(const char* name, Handler&& handler, ChannelSettings<Traits> channelSettings = {}, std::size_t hostInfoMemorySize = 0)
            : m_acceptorHostInfo{ std::make_shared<AcceptorHostInfoMemory>(name, channelSettings, hostInfoMemorySize) },
              m_pingChannel{ create_only, nullptr, m_acceptorHostInfo, channelSettings.GetWaitHandleFactory(), channelSettings.GetReceiverFactory() },
              m_closeEvent{ create_only, true, false, (*m_acceptorHostInfo)->m_acceptorCloseEventName.c_str() },
              m_handler{ std::forward<Handler>(handler) },
              m_channelFactory{ std::move(channelSettings) }
        {
            if (!m_pingChannel.RegisterReceiver(
                [this](detail::ConnectorPingInfo&& pingInfo)
                {
                    try
                    {
                        HostServer(std::move(pingInfo));
                    }
                    catch (...)
                    {
                        std::promise<std::unique_ptr<Connection>> promise;  // TODO: Use custom Maybe<T> in such places to avoid allocation costs in future (?).
                        promise.set_exception(std::current_exception());
                        m_handler(promise.get_future());
                    }
                }))
            {
                throw Exception{ "Failed to register a receiver." };
            }
        }

        Acceptor(const Acceptor& other) = delete;
        Acceptor& operator=(const Acceptor& other) = delete;

        Acceptor(Acceptor&& other) = default;
        Acceptor& operator=(Acceptor&& other) = default;

        ~Acceptor()
        {
            m_closeEvent.Signal();

            m_pingChannel.UnregisterReceiver();
        }

    private:
        class AcceptorHostInfoMemory : public SharedMemory
        {
        public:
            AcceptorHostInfoMemory(const char* name, const ChannelSettings<Traits>& channelSettings, std::size_t hostInfoMemorySize)
                : SharedMemory{ create_only, name, hostInfoMemorySize != 0 ? hostInfoMemorySize : (1 * 1024 * 1024) }
            {
                InvokeAtomic(
                    [&]
                    {
                        auto allocator = GetAllocator<char>();

                        m_info = &Construct<detail::AcceptorHostInfo>(unique_instance, allocator);
                        m_info->m_processId = detail::KernelProcess::GetCurrentProcessId();
                        m_info->m_acceptorCloseEventName.assign(detail::GenerateRandomString().c_str());

                        if (const auto& memory = channelSettings.GetCommonInput())
                        {
                            m_info->m_settings.m_commonInputMemoryName.emplace(memory->GetName().c_str(), allocator);
                        }

                        if (const auto& memory = channelSettings.GetCommonOutput())
                        {
                            m_info->m_settings.m_commonOutputMemoryName.emplace(memory->GetName().c_str(), allocator);
                        }
                    });
            }

            const detail::AcceptorHostInfo* operator->() const
            {
                return m_info;
            }

        private:
            detail::AcceptorHostInfo* m_info{ nullptr };
        };

        using Server = Server<detail::ConnectorInfo, detail::AcceptorInfo, Traits>;

        class AcceptorServer : public Server
        {
        public:
            template <typename Handler, typename CloseHandler>
            AcceptorServer(
                ChannelFactory channelFactory,
                const detail::ConnectorPingInfo& pingInfo,
                Handler&& handler,
                CloseHandler&& closeHandler)
                : AcceptorServer{
                    std::move(channelFactory),
                    pingInfo,
                    detail::KernelEvent{ open_only, pingInfo.m_connectorCloseEventName.c_str() },
                    std::forward<Handler>(handler),
                    std::forward<CloseHandler>(closeHandler) }
            {}

        private:
            template <typename Handler, typename CloseHandler>
            AcceptorServer(
                ChannelFactory&& channelFactory,
                const detail::ConnectorPingInfo& pingInfo,
                detail::KernelEvent&& connectorCloseEvent,
                Handler&& handler,
                CloseHandler&& closeHandler)
            try
                : AcceptorServer{
                    std::move(channelFactory),
                    OpenChannelsOrdered(pingInfo, channelFactory),
                    connectorCloseEvent,
                    detail::KernelProcess{ pingInfo.m_processId },
                    std::forward<Handler>(handler),
                    std::forward<CloseHandler>(closeHandler) }
            {}
            catch (...)
            {
                connectorCloseEvent.Signal();
                throw;
            }

            template <typename Channels, typename Handler, typename CloseHandler>
            AcceptorServer(
                ChannelFactory&& channelFactory,
                Channels&& channels,
                detail::KernelEvent connectorCloseEvent,
                detail::KernelProcess&& process,
                Handler&& handler,
                CloseHandler&& closeHandler)
                : Server{
                    std::make_unique<typename Server::Connection>(
                        nullptr,
                        std::move(connectorCloseEvent),
                        process,
                        channelFactory.GetWaitHandleFactory(),
                        std::move(channels.first),
                        std::move(channels.second)),
                    [this, process, channelFactory, handler = std::forward<Handler>(handler)](
                        detail::ConnectorInfo&& connectorInfo, auto&& callback) mutable
                    {
                        handler(
                            this->GetConnection(),
                            process,
                            std::move(connectorInfo),
                            channelFactory.MakeInstance(),
                            std::forward<decltype(callback)>(callback));
                    },
                    std::forward<CloseHandler>(closeHandler) }
            {}

            static auto OpenChannelsOrdered(const detail::ConnectorPingInfo& pingInfo, const ChannelFactory& channelFactory)
            {
                auto channelFactoryInstance = channelFactory.MakeInstance();

                // Open in a strict order. Must be the reverse of the connector.
                auto input = channelFactoryInstance.template OpenInput<typename Server::InputPacket>(pingInfo.m_connectorInfoChannelName.c_str());
                auto output = channelFactoryInstance.template OpenOutput<typename Server::OutputPacket>(pingInfo.m_acceptorInfoChannelName.c_str());

                return std::make_pair(std::move(input), std::move(output));
            }
        };


        void HostServer(detail::ConnectorPingInfo&& pingInfo)
        {
            m_servers.Accept(
                [&](auto&& closeHandler)
                {
                    return std::make_unique<AcceptorServer>(
                        m_channelFactory.Override(  // Input and output must be flipped.
                            pingInfo.m_settings.m_commonOutputMemoryName
                                ? pingInfo.m_settings.m_commonOutputMemoryName->c_str()
                                : nullptr,
                            pingInfo.m_settings.m_commonInputMemoryName
                                ? pingInfo.m_settings.m_commonInputMemoryName->c_str()
                                : nullptr),
                        pingInfo,
                        [this](
                            const typename AcceptorServer::Connection& connection,
                            const detail::KernelProcess& process,
                            detail::ConnectorInfo&& connectorInfo,
                            typename ChannelFactory::Instance&& channelFactoryInstance,
                            auto&& callback)
                        {
                            std::packaged_task<std::unique_ptr<Connection>()> task{
                                [&]
                                {
                                    return Accept(
                                        process,
                                        detail::AcceptorInfo{ connection.GetOutputChannel().GetMemory()->GetAllocator<char>() },
                                        std::move(connectorInfo),
                                        std::move(channelFactoryInstance),
                                        callback);
                                } };

                            auto result = task.get_future();

                            task();

                            m_handler(std::move(result));
                        },
                        std::forward<decltype(closeHandler)>(closeHandler));
                });
        }

        template <typename Callback>
        std::unique_ptr<Connection> Accept(
            detail::KernelProcess process,
            detail::AcceptorInfo&& acceptorInfo,
            detail::ConnectorInfo&& connectorInfo,
            typename ChannelFactory::Instance&& channelFactoryInstance,
            Callback& callback)
        {
            acceptorInfo.m_closeEventName.assign(detail::GenerateRandomString().c_str());

            return detail::ApplyTuple(
                [&](auto&&... channels)
                {
                    detail::KernelEvent closeEvent{ create_only, false, false, acceptorInfo.m_closeEventName.c_str() };
                    auto connection = std::make_unique<Connection>(
                        closeEvent,
                        closeEvent,
                        std::move(process),
                        channelFactoryInstance.GetWaitHandleFactory(),
                        std::forward<decltype(channels)>(channels)...);

                    callback(std::move(acceptorInfo));

                    return connection;
                },
                CollectChannels(connectorInfo, acceptorInfo, channelFactoryInstance));
        }

        template <typename I = Input, typename O = Output, std::enable_if_t<!std::is_void<I>::value && !std::is_void<O>::value>* = nullptr>
        auto CollectChannels(detail::ConnectorInfo& connectorInfo, detail::AcceptorInfo& acceptorInfo, typename ChannelFactory::Instance& channelFactoryInstance)
        {
            // Make sure opening input channel happens before creating output (important when both must use the same memory).
            auto input = CollectChannels<Input, void>(connectorInfo, acceptorInfo, channelFactoryInstance);
            auto output = CollectChannels<void, Output>(connectorInfo, acceptorInfo, channelFactoryInstance);
            return std::tuple_cat(std::move(input), std::move(output));
        }

        template <typename I = Input, typename O = Output, std::enable_if_t<!std::is_void<I>::value && std::is_void<O>::value>* = nullptr>
        auto CollectChannels(detail::ConnectorInfo& connectorInfo, detail::AcceptorInfo& /*acceptorInfo*/, typename ChannelFactory::Instance& channelFactoryInstance)
        {
            return std::make_tuple(channelFactoryInstance.template OpenInput<Input>(connectorInfo.m_channelName.c_str()));
        }

        template <typename I = Input, typename O = Output, std::enable_if_t<std::is_void<I>::value && !std::is_void<O>::value>* = nullptr>
        auto CollectChannels(detail::ConnectorInfo& /*connectorInfo*/, detail::AcceptorInfo& acceptorInfo, typename ChannelFactory::Instance& channelFactoryInstance)
        {
            acceptorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());
            return std::make_tuple(channelFactoryInstance.template CreateOutput<Output>(acceptorInfo.m_channelName.c_str()));
        }

        std::shared_ptr<AcceptorHostInfoMemory> m_acceptorHostInfo;
        InputChannel<detail::ConnectorPingInfo, Traits> m_pingChannel;
        detail::KernelEvent m_closeEvent;
        detail::Callback<void(std::future<std::unique_ptr<Connection>>)> m_handler;
        ChannelFactory m_channelFactory;
        ComponentCollection<std::list<std::unique_ptr<AcceptorServer>>> m_servers;  // Must be the last member.
    };


    template <typename PacketInfoT> // TODO: Remove the T suffix when VC14 bugs are fixed. Confuses with global PacketInfo struct.
    class PacketAcceptor
        : public Acceptor<typename PacketInfoT::InputPacket, typename PacketInfoT::OutputPacket, typename PacketInfoT::Traits>,
          public PacketInfoT
    {
        using Acceptor<typename PacketInfoT::InputPacket, typename PacketInfoT::OutputPacket, typename PacketInfoT::Traits>::Acceptor;
    };

} // IPC
