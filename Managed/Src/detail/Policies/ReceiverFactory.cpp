#include "IPC/Managed/detail/Policies/ReceiverFactory.h"

#include "Exception.h"
#include "detail/ThreadDetector.h"
#include "IPC/Managed/detail/NativeObject.h"

#include <msclr/gcroot.h>
#include <msclr/auto_handle.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Policies
    {
        ref class Scheduler
        {
        public:
            void Schedule(Interop::Callback<void()>&& callback)
            {
                System::Threading::ThreadPool::QueueUserWorkItem(
                    gcnew System::Threading::WaitCallback(
                        gcnew TaskCallback{ this, std::move(callback) }, &TaskCallback::operator()));
            }

            void Stop()
            {
                m_counter->Signal();

                if (!System::Environment::HasShutdownStarted && !System::AppDomain::CurrentDomain->IsFinalizingForUnload())
                {
                    if (!m_recursionDetector.IsExpectedThread)
                    {
                        m_counter->Wait();
                    }
                }
            }

        internal:
            ref class TaskCallback
            {
            public:
                TaskCallback(Scheduler^ scheduler, Interop::Callback<void()>&& callback)
                    : m_scheduler{ scheduler },
                      m_callback{ std::move(callback) }
                {
                    m_scheduler->m_counter->AddCount();
                }

                ~TaskCallback()
                {
                    m_scheduler->m_counter->Signal();
                }

                void operator()(System::Object^ /*state*/)
                {
                    msclr::auto_handle<TaskCallback> disposer{ this };

                    auto detector = m_scheduler->m_recursionDetector.AutoDetect();

                    (*m_callback)();
                }

            private:
                Scheduler^ m_scheduler;
                NativeObject<Interop::Callback<void()>> m_callback;
            };

        private:
            ThreadDetector m_recursionDetector;
            System::Threading::CountdownEvent^ m_counter{ gcnew System::Threading::CountdownEvent{ 1 } };
        };


        Interop::Policies::ReceiverFactory MakeReceiverFactory()
        {
            return []
            {
                msclr::gcroot<Scheduler^> scheduler{ gcnew Scheduler{} };

                std::shared_ptr<void> disposer{ (void*) true, [scheduler](void*) { scheduler->Stop(); } };

                return [scheduler, disposer = std::move(disposer)](Interop::Callback<void()>&& callback)
                {
                    scheduler->Schedule(std::move(callback));
                };
            };
        }

    } // Policies
    } // detail

} // Managed
} // IPC
