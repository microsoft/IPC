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
        ref class Transport<Request, Response>::ServersAccessor : AccessorBase<IServer^, NativeServersAccessor>, IServersAccessor
        {
        public:
            ServersAccessor(NativeObject<NativeTransport>^ transport, System::String^ name, HandlerFactory^ handlerFactory, System::Boolean enabled)
                : m_transport{ transport },
                  m_name{ name },
                  m_handlerFactory{ handlerFactory }
            {
                Enabled = enabled;
            }

            property System::Collections::Generic::IReadOnlyCollection<IServer^>^ Servers
            {
                virtual System::Collections::Generic::IReadOnlyCollection<IServer^>^ get()
                {
                    auto servers = gcnew System::Collections::Generic::List<IServer^>{};

                    try
                    {
                        for (auto&& item : Accessor()())
                        {
                            auto holder = std::get_deleter<ComponentHolder<Server^>>(item);
                            assert(holder);
                            servers->Add(*holder);
                        }
                    }
                    catch (const std::exception& /*e*/)
                    {
                        ThrowManagedException(std::current_exception());
                    }

                    return gcnew System::Collections::ObjectModel::ReadOnlyCollection<IServer^>{ servers };
                }
            }

        protected:
            NativeServersAccessor MakeAccessor() override
            {
                auto& transport = **m_transport;

                return transport.AcceptServers(
                    msclr::interop::marshal_context().marshal_as<const char*>(m_name),
                    MakeManagedCallback(gcnew ServerFactoryLambda{ m_handlerFactory, transport.GetConfig(), this }),
                    MakeErrorHander(this));
            }

        internal:
            ref class ServerFactoryLambda : ComponentFactoryLambdaBase<NativeServer, NativeConfig, NativeServersAccessor, typename Transport::Server, IServer>
            {
            public:
                ServerFactoryLambda(HandlerFactory^ handlerFactory, const NativeConfig& config, ServersAccessor^ accessor)
                    : ComponentFactoryLambdaBase{ config, accessor },
                      m_handlerFactory{ handlerFactory }
                {}

            protected:
                typename Transport::Server^ MakeComponent(
                    typename NativeServer::ConnectionPtr&& connection,
                    Interop::Callback<void()>&& closeHandler,
                    const NativeConfig& config) override
                {
                    return gcnew Transport::Server{ std::move(connection), m_handlerFactory, std::move(closeHandler), config };
                }

            private:
                HandlerFactory^ m_handlerFactory;
            };

        private:
            NativeObject<NativeTransport>^ m_transport;
            System::String^ m_name;
            HandlerFactory^ m_handlerFactory;
        };


        template <typename Request, typename Response>
        auto Transport<Request, Response>::AcceptServers(System::String^ name, HandlerFactory^ handlerFactory, System::Boolean enabled)
            -> IServersAccessor^
        {
            return gcnew ServersAccessor{ %m_transport, name, handlerFactory, enabled };
        }

    } // detail

} // Managed
} // IPC
