#include "IPC/Managed/detail/Policies/TimeoutFactory.h"
#include "detail/ThreadDetector.h"
#include "IPC/Managed/detail/NativeObject.h"

#include <msclr/gcroot.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Policies
    {
        ref class Timeout
        {
        public:
            Timeout(Interop::Callback<void()> callback)
                : m_callback{ std::make_shared<Interop::Callback<void()>>(std::move(callback)) },
                  m_timer{ gcnew System::Threading::Timer{ gcnew System::Threading::TimerCallback(this, &Timeout::operator()) } }
            {}

            void Stop()
            {
                if (!System::Environment::HasShutdownStarted && !System::AppDomain::CurrentDomain->IsFinalizingForUnload())
                {
                    if (!m_recursionDetector.IsExpectedThread)
                    {
                        System::Threading::ManualResetEvent done{ false };

                        if (m_timer->Dispose(%done))
                        {
                            done.WaitOne();
                        }
                    }

                    (*m_callback).reset();
                }
            }

            void Change(const std::chrono::milliseconds& timeout)
            {
                m_timer->Change(static_cast<int>(timeout.count()), System::Threading::Timeout::Infinite);
            }

        internal:
            void operator()(System::Object^ /*state*/)
            {
                auto detector = m_recursionDetector.AutoDetect();

                if (auto callback = *m_callback)
                {
                    (*callback)();
                }
            }

        private:
            ThreadDetector m_recursionDetector;
            NativeObject<std::shared_ptr<Interop::Callback<void()>>> m_callback;
            System::Threading::Timer^ m_timer;
        };


        Interop::Policies::TimeoutFactory MakeTimeoutFactory(const std::chrono::milliseconds& ms)
        {
            return{
                ms,
                [](Interop::Callback<void()>&& handler)
                {
                    msclr::gcroot<Timeout^> timeout{ gcnew Timeout{ std::move(handler) } };

                    return std::make_pair(
                        std::shared_ptr<void>{ (void*) true, [timeout](void*) { timeout->Stop(); } },
                        [timeout](const std::chrono::milliseconds& ms)
                        {
                            timeout->Change(ms);
                        });
                } };
        }

    } // Policies
    } // detail

} // Managed
} // IPC
