#pragma once

#include "ManagedCallback.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T>
        ref class CloseHandler
        {
        public:
            CloseHandler(T^ component)
                : m_component{ component }
            {}

            void operator()()
            {
                T^ component = nullptr;

                if (m_component.TryGetTarget(component))
                {
                    component->TriggerClosed();
                }
            }

        private:
            System::WeakReference<T^> m_component;
        };


        template <typename T>
        auto MakeCloseHander(T^ component)
        {
            return MakeManagedCallback(gcnew CloseHandler<T>{ component });
        }

    } // detail

} // Managed
} // IPC
