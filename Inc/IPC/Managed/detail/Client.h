#pragma once

#include "Transport.h"
#include "Component.h"
#include "ManagedCallback.h"
#include "Throw.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
#pragma warning(push)
#pragma warning(disable : 4564)
        template <typename Request, typename Response>
        ref class Transport<Request, Response>::Client : Component<NativeClient, IComponent>, IClient
        {
        public:
            virtual System::Threading::Tasks::Task<Response>^ InvokeAsync(Request request, System::TimeSpan timeout)
            {
                auto promise = gcnew System::Threading::Tasks::TaskCompletionSource<Response>{};

                try
                {
                    try
                    {
                        (*m_component)(
                            Cast(request),
                            MakeManagedCallback(gcnew InvokeLambda{ promise }),
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
            Client(typename NativeClient::ConnectionPtr&& connection, const NativeConfig& config)
                : Component{ std::move(connection), nullptr, *config }
            {}

            Client(typename NativeClient::ConnectionPtr&& connection, Interop::Callback<void()>&& closeHandler, const NativeConfig& config)
                : Component{ std::move(connection), std::move(closeHandler), *config }
            {}

            ref struct InvokeLambda
            {
                InvokeLambda(System::Threading::Tasks::TaskCompletionSource<Response>^ promise)
                    : m_promise{ promise }
                {}

                ~InvokeLambda()
                {
                    m_promise->TrySetCanceled();
                }

                void operator()(NativeResponse&& response)
                {
                    try
                    {
                        try
                        {
                            m_promise->SetResult(Cast(response));
                        }
                        catch (const std::exception& /*e*/)
                        {
                            ThrowManagedException(std::current_exception());
                        }
                    }
                    catch (System::Exception^ e)
                    {
                        m_promise->SetException(e);
                    }
                }

                System::Threading::Tasks::TaskCompletionSource<Response>^ m_promise;
            };
        };
#pragma warning(pop)

    } // detail

} // Managed
} // IPC
