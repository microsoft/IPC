#pragma once

#include "Transport.h"
#include "NativeObject.h"
#include "ComponentHolder.h"
#include "ManagedCallback.h"
#include "AccessorBase.h"

#include <msclr/marshal.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename Request, typename Response>
        ref class Transport<Request, Response>::ClientAccessor : AccessorBase<IClient^>, IClientAccessor
        {
        public:
            ClientAccessor(NativeTransport& transport, System::String^ acceptorName, System::Boolean async, System::TimeSpan timeout, IClientConnector^ connector)
            try
                : m_accessor{ transport.ConnectClient(
                    msclr::interop::marshal_context().marshal_as<const char*>(acceptorName),
                    *safe_cast<ClientConnector^>(connector)->m_connector,
                    async,
                    MakeManagedCallback(gcnew ClientFactoryLambda{ transport.GetConfig(), this }),
                    MakeErrorHander(this),
                    std::chrono::milliseconds{ static_cast<std::size_t>(timeout.TotalMilliseconds) }) }
            {}
            catch (const std::exception& /*e*/)
            {
                ThrowManagedException(std::current_exception());
            }

            property IClient^ Client
            {
                virtual IClient^ get()
                {
                    try
                    {
                        auto holder = std::get_deleter<ComponentHolder<Transport::Client^>>((*m_accessor)());
                        assert(holder);
                        return *holder;
                    }
                    catch (const std::exception& /*e*/)
                    {
                        ThrowManagedException(std::current_exception());
                    }
                }
            }

        internal:
            ref class ClientFactoryLambda : ComponentFactoryLambdaBase<NativeClient, NativeConfig, typename Transport::Client, IClient>
            {
            public:
                ClientFactoryLambda(const NativeConfig& config, ClientAccessor^ accessor)
                    : ClientFactoryLambda::ComponentFactoryLambdaBase{ config, accessor }
                {}

            protected:
                typename Transport::Client^ MakeComponent(
                    typename NativeClient::ConnectionPtr&& connection,
                    Interop::Callback<void()>&& closeHandler,
                    const NativeConfig& config) override
                {
                    return gcnew Transport::Client{ std::move(connection), std::move(closeHandler), config };
                }
            };

        private:
            NativeObject<Interop::Callback<std::shared_ptr<void>()>> m_accessor;
        };


        template <typename Request, typename Response>
        auto Transport<Request, Response>::ConnectClient(System::String^ acceptorName, System::Boolean async, System::TimeSpan timeout, IClientConnector^ connector)
            -> IClientAccessor^
        {
            return gcnew ClientAccessor{ *m_transport, acceptorName, async, timeout, connector ? connector : m_clientConnector.Value };
        }

    } // detail

} // Managed
} // IPC
