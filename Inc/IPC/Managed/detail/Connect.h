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
        ref class Transport<Request, Response>::ClientAccessor : AccessorBase<IClient^, NativeClientAccessor>, IClientAccessor
        {
        public:
            ClientAccessor(
                NativeObject<NativeTransport>^ transport, System::String^ name, System::Boolean async, System::TimeSpan timeout, IClientConnector^ connector, System::Boolean enabled)
                : m_transport{ transport },
                  m_name{ name },
                  m_async{ async },
                  m_timeout{ timeout },
                  m_connector{ connector }
            {
                Enabled = enabled;
            }

            property IClient^ Client
            {
                virtual IClient^ get()
                {
                    try
                    {
                        auto holder = std::get_deleter<ComponentHolder<Transport::Client^>>(Accessor());
                        assert(holder);
                        return *holder;
                    }
                    catch (const std::exception& /*e*/)
                    {
                        ThrowManagedException(std::current_exception());
                    }
                }
            }

        protected:
            NativeClientAccessor MakeAccessor() override
            {
                auto& transport = **m_transport;

                return transport.ConnectClient(
                    msclr::interop::marshal_context().marshal_as<const char*>(m_name),
                    *safe_cast<ClientConnector^>(m_connector)->m_connector,
                    m_async,
                    MakeManagedCallback(gcnew ClientFactoryLambda{ transport.GetConfig(), this }),
                    MakeErrorHander(this),
                    std::chrono::milliseconds{ static_cast<std::size_t>(m_timeout.TotalMilliseconds) });
            }

        internal:
            ref class ClientFactoryLambda : ComponentFactoryLambdaBase<NativeClient, NativeConfig, NativeClientAccessor, typename Transport::Client, IClient>
            {
            public:
                ClientFactoryLambda(const NativeConfig& config, ClientAccessor^ accessor)
                    : ComponentFactoryLambdaBase{ config, accessor }
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
            NativeObject<NativeTransport>^ m_transport;
            System::String^ m_name;
            System::Boolean m_async;
            System::TimeSpan m_timeout;
            IClientConnector^ m_connector;
        };


        template <typename Request, typename Response>
        auto Transport<Request, Response>::ConnectClient(
            System::String^ acceptorName, System::Boolean async, System::TimeSpan timeout, IClientConnector^ connector, System::Boolean enabled)
            -> IClientAccessor^
        {
            return gcnew ClientAccessor{ %m_transport, acceptorName, async, timeout, connector ? connector : m_clientConnector.Value, enabled };
        }

    } // detail

} // Managed
} // IPC
