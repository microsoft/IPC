#include "IPC/Managed/detail/Policies/WaitHandleFactory.h"
#include "detail/ThreadDetector.h"
#include "IPC/Managed/detail/NativeObject.h"

#include <msclr/gcroot.h>

#pragma managed(push, off)
#include <windows.h>
#include <memory>
#pragma managed(pop)



namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Policies
    {
        ref class Waiter
        {
        public:
            Waiter(HANDLE handle, Interop::Callback<bool()>&& callback)
                : m_handle{ handle },
                  m_callback{ std::make_shared<Interop::Callback<bool()>>(std::move(callback)) }
            {}

            void Monitor()
            {
                System::Threading::Interlocked::Exchange(
                    m_registeredHandle,
                    System::Threading::ThreadPool::RegisterWaitForSingleObject(
                        %m_handle,
                        gcnew System::Threading::WaitOrTimerCallback(this, &Waiter::operator()),
                        nullptr,
                        -1,
                        true)); // No overlapping.
            }

            void Stop()
            {
                System::Threading::Interlocked::Exchange(m_stop, 1);

                if (!System::Environment::HasShutdownStarted && !System::AppDomain::CurrentDomain->IsFinalizingForUnload())
                {
                    if (!m_recursionDetector.IsExpectedThread)
                    {
                        Unregister(true);   // Wait for threads that did not observe m_stop.
                        Unregister(true);   // Wait for threads that did observe m_stop.
                    }
                    else
                    {
                        Unregister(false);
                    }

                    (*m_callback).reset();
                }
            }

        internal:
            void operator()(System::Object^ /*state*/, System::Boolean /*timedOut*/)
            {
                auto detector = m_recursionDetector.AutoDetect();

                if (auto callback = *m_callback)
                {
                    if ((*callback)() && !System::Threading::Interlocked::Read(m_stop)) // The m_stop must be checked last for recursive case.
                    {
                        Monitor();
                    }
                }
            }

        private:
            ref class Win32WaitHandle : System::Threading::WaitHandle
            {
            public:
                Win32WaitHandle(HANDLE handle)
                {
                    HANDLE newHandle;
                    ::DuplicateHandle(
                        ::GetCurrentProcess(),
                        handle,
                        ::GetCurrentProcess(),
                        &newHandle,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS);

                    SafeWaitHandle = gcnew Microsoft::Win32::SafeHandles::SafeWaitHandle{ System::IntPtr{ newHandle }, true };
                }
            };

            void Unregister(bool wait)
            {
                auto handle = System::Threading::Interlocked::CompareExchange<System::Threading::RegisteredWaitHandle^>(
                    m_registeredHandle, nullptr, nullptr);

                if (wait)
                {
                    System::Threading::ManualResetEvent done{ false };

                    if (handle->Unregister(%done))
                    {
                        done.WaitOne();
                    }
                }
                else
                {
                    handle->Unregister(nullptr);
                }
            }


            ThreadDetector m_recursionDetector;
            Win32WaitHandle m_handle;
            NativeObject<std::shared_ptr<Interop::Callback<bool()>>> m_callback;
            System::Int64 m_stop{ 0 };
            System::Threading::RegisteredWaitHandle^ m_registeredHandle;
        };


        Interop::Policies::WaitHandleFactory MakeWaitHandleFactory()
        {
            return [](const IPC::detail::KernelObject& obj, Interop::Callback<bool()> handler)
            {
                msclr::gcroot<Waiter^> waiter{ gcnew Waiter{ static_cast<void*>(obj), std::move(handler) } };

                waiter->Monitor();

                return std::shared_ptr<void>{ (void*) true, [waiter](void*) { waiter->Stop(); } };
            };
        }

    } // Policies
    } // detail

} // Managed
} // IPC
