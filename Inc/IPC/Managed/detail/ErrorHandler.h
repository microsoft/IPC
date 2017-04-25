#pragma once

#include "ManagedCallback.h"
#include "Throw.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T>
        ref class ErrorHandler
        {
        public:
            ErrorHandler(T^ component)
                : m_component{ component }
            {}

            void operator()(std::exception_ptr&& error)
            {
                try
                {
                    ThrowManagedException(error);
                }
                catch (System::Exception^ e)
                {
                    operator()(e);
                }
            }

            void operator()(System::Exception^ e)
            {
                if (auto component = this->Component)
                {
                    component->TriggerError(e);
                }
            }

            property T^ Component
            {
                T^ get()
                {
                    T^ component;
                    return m_component.TryGetTarget(component) ? component : nullptr;
                }
            }

        private:
            System::WeakReference<T^> m_component;
        };


        template <typename T>
        auto MakeErrorHander(T^ component)
        {
            return MakeManagedCallback(gcnew ErrorHandler<T>{ component });
        }

    } // detail

} // Managed
} // IPC
