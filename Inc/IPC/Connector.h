#pragma once

#include "ConnectorFwd.h"
#include "Client.h"
#include "DefaultTraits.h"
#include "ComponentCollection.h"
#include "detail/ChannelFactory.h"
#include "detail/Info.h"
#include "detail/KernelProcess.h"
#include "detail/RandomString.h"
#include "detail/Apply.h"
#include <memory>
#include <string>
#include <map>
#include <type_traits>
#include <future>


namespace IPC
{
    namespace detail
    {
        template <typename K, typename V, typename Comparer, typename Allocator>
        struct IsStable<std::map<K, V, Comparer, Allocator>> : std::true_type
        {};


        template <typename K, typename V, typename Comparer, typename Allocator>
        class ComponentCollectionInserter<std::map<K, V, Comparer, Allocator>>
        {
        public:
            explicit ComponentCollectionInserter(std::map<K, V, Comparer, Allocator>& container)
                : m_container{ &container }
            {}

            template <typename U>
            auto operator()(U&& value)
            {
                auto result = m_container->insert(std::forward<U>(value));

                return result.second ? result.first : m_container->end();
            }

        private:
            std::map<K, V, Comparer, Allocator>* m_container;
        };

    } // detail


    template <typename Input, typename Output, typename Traits>
    class Connector
    {
        using ChannelFactory = detail::ChannelFactory<Traits>;

    public:
        using Connection = Connection<Input, Output, Traits>;

        Connector()
            : Connector{ {} }
        {}

        explicit Connector(ChannelSettings<Traits> channelSettings, typename Traits::TransactionManagerFactory transactionManagerFactory = {})
            : m_transactionManagerFactory{ std::move(transactionManagerFactory) },
              m_channelFactory{ std::move(channelSettings) }
        {}

        Connector(const Connector& other) = delete;
        Connector& operator=(const Connector& other) = delete;

        Connector(Connector&& other) = default;
        Connector& operator=(Connector&& other) = default;

#ifdef NDEBUG
        ~Connector() = default;
#else
        ~Connector()
        {
            if (m_clients)
            {
                auto clients = m_clients.GetComponents();
                for (auto& entry : *clients)
                {
                    assert(entry.second.unique());  // No one must be holding an extra ref on clients.
                }
            }
        }
#endif

        template <typename Callback, typename... TransactionArgs, typename U = std::future<std::unique_ptr<Connection>>,
            decltype(std::declval<Callback>()(std::declval<U>()))* = nullptr>
        void Connect(const char* acceptorName, Callback&& callback, TransactionArgs&&... transactionArgs)
        {
            class GuaranteedCallback
            {
            public:
                explicit GuaranteedCallback(Callback&& callback)
                    : m_callback{ std::forward<Callback>(callback) }
                {}

                ~GuaranteedCallback()
                {
                    if (!m_invoked)
                    {
                        operator()(std::make_exception_ptr(Exception{ "Acceptor is closed." }));
                    }
                }

                void operator()(std::exception_ptr error)
                {
                    std::promise<std::unique_ptr<Connection>> promise;
                    promise.set_exception(error);
                    operator()(promise.get_future());
                }

                void operator()(std::future<std::unique_ptr<Connection>>&& connection)
                {
                    m_invoked = true;
                    m_callback(std::move(connection));
                }

            private:
                std::decay_t<Callback> m_callback;
                bool m_invoked{ false };
            };

            auto guaranteedCallback = std::make_shared<GuaranteedCallback>(std::forward<Callback>(callback));

            try
            {
                InvokeAcceptor(
                    BeginConnect(acceptorName),
                    [this, guaranteedCallback](auto&& state) mutable
                    {
                        std::packaged_task<std::unique_ptr<Connection>()> task{ [&] { return EndConnect(state); } };
                        auto result = task.get_future();

                        task();

                        (*guaranteedCallback)(std::move(result));
                    },
                    std::forward<TransactionArgs>(transactionArgs)...);
            }
            catch (...)
            {
                (*guaranteedCallback)(std::current_exception());
            }
        }

        template <typename... TransactionArgs>
        std::future<std::unique_ptr<Connection>> Connect(const char* acceptorName, TransactionArgs&&... transactionArgs)
        {
            using ConnectionPtr = std::unique_ptr<Connection>;

            std::packaged_task<ConnectionPtr(std::future<ConnectionPtr>)> callback{ [](auto&& connection) { return connection.get(); } };
            auto result = callback.get_future();

            Connect(acceptorName, std::move(callback), std::forward<TransactionArgs>(transactionArgs)...);

            return result;
        }

    private:
        class ConnectorClient : public Client<detail::ConnectorInfo, detail::AcceptorInfo, Traits>
        {
        public:
            ConnectorClient(
                ChannelFactory channelFactory,
                const detail::AcceptorHostInfo& acceptorHostInfo,
                const detail::ConnectorPingInfo& pingInfo,
                typename Traits::TransactionManagerFactory transactionManagerFactory,
                detail::Callback<void()> closeHandler)
                : ConnectorClient{
                    std::move(channelFactory),
                    CreateChannelsOrdered(pingInfo, channelFactory),
                    acceptorHostInfo,
                    pingInfo,
                    std::move(transactionManagerFactory),
                    std::move(closeHandler),
                    detail::KernelProcess{ acceptorHostInfo.m_processId } }
            {}

            const detail::KernelProcess& GetProcess() const
            {
                return m_process;
            }

            const auto& GetChannelFactory() const
            {
                return m_channelFactory;
            }

        private:
            template <typename Channels>
            ConnectorClient(
                ChannelFactory&& channelFactory,
                Channels&& channels,
                const detail::AcceptorHostInfo& acceptorHostInfo,
                const detail::ConnectorPingInfo& pingInfo,
                typename Traits::TransactionManagerFactory&& transactionManagerFactory,
                detail::Callback<void()>&& closeHandler,
                detail::KernelProcess process)
                : ConnectorClient::Client{
                    std::make_unique<typename ConnectorClient::Connection>(
                        detail::KernelEvent{ create_only, false, false, pingInfo.m_connectorCloseEventName.c_str() },
                        true,
                        detail::KernelEvent{ open_only, acceptorHostInfo.m_acceptorCloseEventName.c_str() },
                        process,
                        channelFactory.GetWaitHandleFactory(),
                        std::move(channels.first),
                        std::move(channels.second)),
                    std::move(closeHandler),
                    std::move(transactionManagerFactory)(detail::Identity<typename ConnectorClient::Client::TransactionManager>{}) },
                  m_process{ std::move(process) },
                  m_channelFactory{ std::move(channelFactory) }
            {}

            static auto CreateChannelsOrdered(const detail::ConnectorPingInfo& pingInfo, const ChannelFactory& channelFactory)
            {
                auto channelFactoryInstance = channelFactory.MakeInstance();

                // Create in a strict order. Must be the reverse of the acceptor.
                auto output = channelFactoryInstance.template CreateOutput<typename ConnectorClient::Client::OutputPacket>(
                    pingInfo.m_connectorInfoChannelName.c_str());
                auto input = channelFactoryInstance.template CreateInput<typename ConnectorClient::Client::InputPacket>(
                    pingInfo.m_acceptorInfoChannelName.c_str());

                return std::make_pair(std::move(input), std::move(output));
            }


            detail::KernelProcess m_process;
            ChannelFactory m_channelFactory;
        };

        std::shared_ptr<ConnectorClient> TryCreateClient(const char* acceptorName)
        {
            auto acceptorInfoMemory = std::make_shared<SharedMemory>(open_only, acceptorName);
            const auto& acceptorHostInfo = acceptorInfoMemory->Find<detail::AcceptorHostInfo>(unique_instance);

            OutputChannel<detail::ConnectorPingInfo, Traits> pingChannel{ open_only, nullptr, acceptorInfoMemory };
            auto pingInfo = MakePingInfo(pingChannel.GetMemory()->GetAllocator<char>());

            std::shared_ptr<ConnectorClient> client;

            auto result = m_clients.Accept(
                [&](auto&& closeHandler)
                {
                    client = std::make_shared<ConnectorClient>(
                        m_channelFactory.Override(  // Input and output must be flipped.
                            acceptorHostInfo.m_settings.m_commonOutputMemoryName
                                ? acceptorHostInfo.m_settings.m_commonOutputMemoryName->c_str()
                                : nullptr,
                            acceptorHostInfo.m_settings.m_commonInputMemoryName
                                ? acceptorHostInfo.m_settings.m_commonInputMemoryName->c_str()
                                : nullptr),
                        acceptorHostInfo,
                        pingInfo,
                        m_transactionManagerFactory,
                        std::forward<decltype(closeHandler)>(closeHandler));

                    return std::make_pair(acceptorName, client);
                });

            if (result)
            {
                pingChannel.Send(pingInfo);
                return client;
            }

            return{};
        }

        std::shared_ptr<ConnectorClient> TryGetClient(const char* acceptorName)
        {
            {
                auto clients = m_clients.GetComponents();

                auto it = clients->find(acceptorName);
                if (it != clients->end())
                {
                    return it->second;
                }
            }

            return TryCreateClient(acceptorName);
        }

        detail::ConnectorPingInfo MakePingInfo(const SharedMemory::Allocator<char>& allocator)
        {
            detail::ConnectorPingInfo pingInfo{ allocator };

            pingInfo.m_processId = detail::KernelProcess::GetCurrentProcessId();
            pingInfo.m_connectorCloseEventName.assign(detail::GenerateRandomString().c_str());
            pingInfo.m_acceptorInfoChannelName.assign(detail::GenerateRandomString().c_str());
            pingInfo.m_connectorInfoChannelName.assign(detail::GenerateRandomString().c_str());

            if (const auto& memory = m_channelFactory.GetCommonInput())
            {
                pingInfo.m_settings.m_commonInputMemoryName.emplace(memory->GetName().c_str(), allocator);
            }

            if (const auto& memory = m_channelFactory.GetCommonOutput())
            {
                pingInfo.m_settings.m_commonOutputMemoryName.emplace(memory->GetName().c_str(), allocator);
            }

            return pingInfo;
        }

        std::shared_ptr<ConnectorClient> GetClient(const char* acceptorName)
        {
            std::shared_ptr<ConnectorClient> client;

            do
            {
                client = TryGetClient(acceptorName);

            } while (!client);

            return client;
        }

        template <typename O = Output, std::enable_if_t<std::is_void<O>::value>* = nullptr>
        auto BeginConnect(const char* acceptorName)
        {
            auto client = GetClient(acceptorName);
            auto memory = client->GetConnection().GetOutputChannel().GetMemory();
            auto process = client->GetProcess();
            auto channelFactoryInstance = client->GetChannelFactory().MakeInstance();

            return std::make_tuple(
                std::move(client),
                std::move(process),
                detail::ConnectorInfo{ memory->GetAllocator<char>() },
                std::move(channelFactoryInstance));
        }

        template <typename O = Output, std::enable_if_t<!std::is_void<O>::value>* = nullptr>
        auto BeginConnect(const char* acceptorName)
        {
            auto state = BeginConnect<void>(acceptorName);

            auto& connectorInfo = std::get<detail::ConnectorInfo>(state);
            connectorInfo.m_channelName.assign(detail::GenerateRandomString().c_str());

            auto& transaction = std::get<typename ChannelFactory::Instance>(state);

            return std::tuple_cat(std::move(state), std::make_tuple(transaction.template CreateOutput<Output>(connectorInfo.m_channelName.c_str())));
        }

        template <typename State, typename Callback, typename... TransactionArgs>
        void InvokeAcceptor(State&& state, Callback&& callback, TransactionArgs&&... transactionArgs)
        {
            auto client = std::move(std::get<std::shared_ptr<ConnectorClient>>(state)); // Move the client out of the state!

            auto connectorInfo = std::move(std::get<detail::ConnectorInfo>(state));

            (*client)(
                std::move(connectorInfo),
                [state = std::forward<State>(state), callback = std::forward<Callback>(callback)](detail::AcceptorInfo&& acceptorInfo) mutable
                {
                    callback(std::tuple_cat(std::forward<State>(state), std::make_tuple(std::move(acceptorInfo))));
                },
                std::forward<TransactionArgs>(transactionArgs)...);
        }

        template <typename State>
        std::unique_ptr<Connection> EndConnect(State& state)
        {
            detail::KernelEvent closeEvent{ open_only, std::get<detail::AcceptorInfo>(state).m_closeEventName.c_str() };

            try
            {
                return detail::ApplyTuple(
                    [&](auto&&... channels)
                    {
                        return std::make_unique<Connection>(
                            closeEvent,
                            closeEvent,
                            std::move(std::get<detail::KernelProcess>(state)),
                            std::get<typename ChannelFactory::Instance>(state).GetWaitHandleFactory(),
                            std::forward<decltype(channels)>(channels)...);
                    },
                    CollectChannels(state));
            }
            catch (...)
            {
                closeEvent.Signal();
                throw;
            }
        }

        template <typename I = Input, typename O = Output, typename State, std::enable_if_t<!std::is_void<I>::value && !std::is_void<O>::value>* = nullptr>
        auto CollectChannels(State& state)
        {
            return std::tuple_cat(CollectChannels<Input, void>(state), CollectChannels<void, Output>(state));
        }

        template <typename I = Input, typename O = Output, typename State, std::enable_if_t<!std::is_void<I>::value && std::is_void<O>::value>* = nullptr>
        auto CollectChannels(State& state)
        {
            return std::make_tuple(std::get<typename ChannelFactory::Instance>(state).template OpenInput<Input>(
                std::get<detail::AcceptorInfo>(state).m_channelName.c_str()));
        }

        template <typename I = Input, typename O = Output, typename State, std::enable_if_t<std::is_void<I>::value && !std::is_void<O>::value>* = nullptr>
        auto CollectChannels(State& state)
        {
            return std::make_tuple(std::move(std::get<OutputChannel<Output, Traits>>(state)));
        }


        typename Traits::TransactionManagerFactory m_transactionManagerFactory;
        ChannelFactory m_channelFactory;
        ComponentCollection<std::map<std::string, std::shared_ptr<ConnectorClient>>> m_clients; // Must be the last member.
    };


    template <typename PacketInfoT> // TODO: Remove the T suffix when VC14 bugs are fixed. Confuses with global PacketInfo struct.
    class PacketConnector
        : public Connector<typename PacketInfoT::InputPacket, typename PacketInfoT::OutputPacket, typename PacketInfoT::Traits>,
          public PacketInfoT
    {
    public:
        template <typename... Args>
        PacketConnector(Args&&... args)                     // TODO: Inherit constructors instead when VC14 bugs are fixed.
            : PacketConnector::Connector{ std::forward<Args>(args)... }
        {}
    };

} // IPC
