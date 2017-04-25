#pragma once

#include "Transport.h"
#include "Client.h"
#include "NativeObject.h"
#include "ManagedCallback.h"

#include <msclr/marshal.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
#pragma warning(push)
#pragma warning(disable : 4564)
        template <typename Request, typename Response>
        ref class Transport<Request, Response>::ClientConnector : IClientConnector
        {
        public:
            virtual System::Threading::Tasks::Task<IClient^>^ ConnectAsync(System::String^ acceptorName, System::TimeSpan timeout)
            {
                auto promise = gcnew System::Threading::Tasks::TaskCompletionSource<IClient^>{};

                try
                {
                    try
                    {
                        m_connector->Connect(
                            msclr::interop::marshal_context().marshal_as<const char*>(acceptorName),
                            MakeManagedCallback(gcnew ConnectLambda{ promise, *m_config }),
                            std::chrono::milliseconds{ static_cast<std::chrono::milliseconds::rep>(timeout.TotalMilliseconds) });
                    }
                    catch (const std::exception& /*e*/)
                    {
                        ThrowManagedException(std::current_exception());
                    }
                }
                catch (System::Exception^ e)
                {
                    promise->TrySetException(e);
                }

                return promise->Task;
            }

        internal:
            ClientConnector(const NativeConfig& config)
                : m_connector{ *config },
                  m_config{ config }
            {}

            ref struct ConnectLambda
            {
                ConnectLambda(System::Threading::Tasks::TaskCompletionSource<IClient^>^ promise, const NativeConfig& config)
                    : m_promise{ promise },
                      m_config{ config }
                {}

                ~ConnectLambda()
                {
                    m_promise->TrySetCanceled();
                }

                void operator()(Interop::Callback<typename NativeClient::ConnectionPtr()>&& getConnection)
                {
                    try
                    {
                        Client^ client = nullptr;

                        try
                        {
                            client = gcnew Client{ getConnection(), *m_config };
                        }
                        catch (const std::exception& /*e*/)
                        {
                            ThrowManagedException(std::current_exception());
                        }

                        m_promise->SetResult(client);
                    }
                    catch (System::Exception^ e)
                    {
                        m_promise->SetException(e);
                    }
                }

                System::Threading::Tasks::TaskCompletionSource<IClient^>^ m_promise;
                NativeObject<NativeConfig> m_config;
            };

            NativeObject<NativeClientConnector> m_connector;
            NativeObject<NativeConfig> m_config;
        };
#pragma warning(pop)

    } // detail

} // Managed
} // IPC
