#pragma once

#include "Transport.h"
#include "Server.h"
#include "NativeObject.h"
#include "ManagedCallback.h"

#include <msclr/marshal.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename Request, typename Response>
        ref class Transport<Request, Response>::ServerAcceptor : IServerAcceptor
        {
        public:
            ServerAcceptor(System::String^ name, HandlerFactory^ handlerFactory, const NativeConfig& config)
                : m_acceptor{
                    msclr::interop::marshal_context().marshal_as<const char*>(name),
                    MakeManagedCallback(gcnew AcceptedLambda{ this, config }),
                    *config },
                  m_handlerFactory{ handlerFactory }
            {}

            virtual event System::EventHandler<ComponentEventArgs<IServer^>^>^ Accepted;

            virtual event System::EventHandler<ErrorEventArgs^>^ Error;

        internal:
            ref struct AcceptedLambda
            {
                AcceptedLambda(ServerAcceptor^ acceptor, const NativeConfig& config)
                    : m_acceptor{ acceptor },
                      m_config{ config }
                {}

                void operator()(Interop::Callback<typename NativeServer::ConnectionPtr()>&& getConnection)
                {
                    try
                    {
                        Server^ server = nullptr;

                        try
                        {
                            server = gcnew Server{ getConnection(), m_acceptor->m_handlerFactory, *m_config };
                        }
                        catch (const std::exception& /*e*/)
                        {
                            ThrowManagedException(std::current_exception());
                        }

                        m_acceptor->Accepted(m_acceptor, gcnew ComponentEventArgs<IServer^>{ server });
                    }
                    catch (System::Exception^ e)
                    {
                        m_acceptor->Error(m_acceptor, gcnew ErrorEventArgs{ e });
                    }
                }

                ServerAcceptor^ m_acceptor;
                NativeObject<NativeConfig> m_config;
            };

        private:
            NativeObject<NativeServerAcceptor> m_acceptor;
            HandlerFactory^ m_handlerFactory;
        };

    } // detail

} // Managed
} // IPC
