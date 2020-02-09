#pragma once

#include "Transport.h"
#include "Component.h"
#include <msclr/auto_handle.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        namespace
        {
            template <typename T>
            msclr::auto_handle<T> MakeAutoHandle(T^ obj)
            {
                return msclr::auto_handle<T>{ obj };
            }

            template <typename T>
            T% MakeAutoHandle(T% obj)
            {
                return obj;
            }

        } // anonymous


        template <typename Request, typename Response>
        ref class Transport<Request, Response>::Server : Component<NativeServer, IComponent>, IServer
        {
        public:
            virtual event System::EventHandler<ErrorEventArgs^>^ Error;

        internal:
            Server(typename NativeServer::ConnectionPtr&& connection, HandlerFactory^ handlerFactory, const NativeConfig& config)
                : Server::Component{ std::move(connection), nullptr, MakeHandlerFactory(connection, handlerFactory), *config }
            {}

            Server(typename NativeServer::ConnectionPtr&& connection, HandlerFactory^ handlerFactory, Interop::Callback<void()>&& closeHandler, const NativeConfig& config)
                : Server::Component{ std::move(connection), std::move(closeHandler), MakeHandlerFactory(connection, handlerFactory), *config }
            {}

            ~Server()
            {
                m_pendingTasks->Signal();
                m_pendingTasks->Wait();
            }

            ref struct HandlerLambda
            {
                HandlerLambda(Handler^ handler, Server^ server)
                    : m_handler{ handler },
                      m_server{ server }
                {}

                void operator()(NativeRequest&& request, Interop::Callback<void(const NativeResponse&)>&& callback)
                {
                    Server^ server = nullptr;

                    if (m_server.TryGetTarget(server) && server->m_pendingTasks->TryAddCount())
                    {
                        try
                        {
                            m_handler(Cast(request))->ContinueWith(
                                gcnew System::Action<System::Threading::Tasks::Task<Response>^>(
                                    gcnew ResponseLambda{ std::move(callback), server }, &ResponseLambda::operator()),
                                System::Threading::Tasks::TaskContinuationOptions::ExecuteSynchronously);
                        }
                        catch (System::Exception^ e)
                        {
                            server->m_pendingTasks->Signal();
                            server->Error(server, gcnew ErrorEventArgs{ e });
                        }
                    }
                }

                Handler^ m_handler;
                System::WeakReference<Server^> m_server;
            };

            ref struct ResponseLambda
            {
                ResponseLambda(Interop::Callback<void(const NativeResponse&)>&& callback, Server^ server)
                    : m_callback{ std::move(callback) },
                      m_server{ server }
                {}

                ~ResponseLambda()
                {
                    m_server->m_pendingTasks->Signal();
                }

                void operator()(System::Threading::Tasks::Task<Response>^ task)
                {
                    msclr::auto_handle<ResponseLambda> selfDisposer{ this };

                    try
                    {
                        auto response = task->Result;
                        auto responseDisposer = MakeAutoHandle(response);
                        (void)responseDisposer;

                        try
                        {
                            (*m_callback)(Cast(response));
                        }
                        catch (const std::exception& /*e*/)
                        {
                            ThrowManagedException(std::current_exception());
                        }
                    }
                    catch (System::Exception^ e)
                    {
                        m_server->Error(m_server, gcnew ErrorEventArgs{ e });
                    }
                }

                NativeObject<Interop::Callback<void(const NativeResponse&)>> m_callback;
                Server^ m_server;
            };

        private:
            auto MakeHandlerFactory(const typename NativeServer::ConnectionPtr& connection, HandlerFactory^ handlerFactory)
            {
                return MakeManagedCallback(gcnew HandlerLambda{
                    handlerFactory(gcnew SharedMemory{ connection.GetInputMemory() }, gcnew SharedMemory{ connection.GetOutputMemory() }),
                    this });
            }

            System::Threading::CountdownEvent^ m_pendingTasks{ gcnew System::Threading::CountdownEvent{ 1 } };
        };

    } // detail

} // Managed
} // IPC
