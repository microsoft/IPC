#pragma once

#include "Transport.h"
#include "NativeObject.h"
#include "ComponentHolder.h"
#include "ManagedCallback.h"
#include "AccessorBase.h"

#pragma managed(push, off)
#include <boost/optional.hpp>
#pragma managed(pop)

#include <msclr/marshal.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename Request, typename Response>
        ref class Transport<Request, Response>::ServersAccessor : AccessorBase<IServer^>, IServersAccessor
        {
        public:
            ServersAccessor(NativeTransport& transport, System::String^ name, HandlerFactory^ handlerFactory)
            try
                : m_accessor{ transport.AcceptServers(
                    msclr::interop::marshal_context().marshal_as<const char*>(name),
                    MakeManagedCallback(gcnew ServerFactoryLambda{ handlerFactory, transport.GetConfig(), this }),
                    MakeErrorHander(this)) }
            {}
            catch (const std::exception& /*e*/)
            {
                ThrowManagedException(std::current_exception());
            }

            property System::Collections::Generic::IReadOnlyCollection<IServer^>^ Servers
            {
                virtual System::Collections::Generic::IReadOnlyCollection<IServer^>^ get()
                {
                    auto servers = gcnew System::Collections::Generic::List<IServer^>{};

                    try
                    {
                        for (auto&& item : (*m_accessor)()())
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

        internal:
            ref class ServerFactoryLambda : ComponentFactoryLambdaBase<NativeServer, NativeConfig, typename Transport::Server, IServer>
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
            NativeObject<Interop::Callback<Interop::Callback<const NativeServerCollection&()>()>> m_accessor;
        };


        template <typename Request, typename Response>
        auto Transport<Request, Response>::AcceptServers(System::String^ name, HandlerFactory^ handlerFactory)
            -> IServersAccessor^
        {
            return gcnew ServersAccessor{ *m_transport, name, handlerFactory };
        }

    } // detail

} // Managed
} // IPC
