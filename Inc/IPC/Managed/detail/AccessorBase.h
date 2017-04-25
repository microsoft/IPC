#pragma once

#include "NativeObject.h"
#include "ComponentHolder.h"
#include "ErrorHandler.h"
#include "Interop/Callback.h"

#include <msclr/auto_handle.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T>
        ref class AccessorBase : IAccessor<T>
        {
        public:
            ~AccessorBase()
            {}

            virtual event System::EventHandler<ComponentEventArgs<T>^>^ Connected;

            virtual event System::EventHandler<ComponentEventArgs<T>^>^ Disconnected;

            virtual event System::EventHandler<ErrorEventArgs^>^ Error;

        protected:
            AccessorBase()
            {}

        internal:
            void TriggerConnected(T component)
            {
                Connected(this, gcnew ComponentEventArgs<T>{ component });
            }

            void TriggerDisconnected(T component)
            {
                Disconnected(this, gcnew ComponentEventArgs<T>{ component });
            }

            void TriggerError(System::Exception^ e)
            {
                Error(this, gcnew ErrorEventArgs{ e });
            }
        };


        template <typename NativeComponent, typename NativeConfig, typename Component, typename Interface>
        ref class ComponentFactoryLambdaBase abstract
        {
        public:
            std::shared_ptr<void> operator()(typename NativeComponent::ConnectionPtr&& connection, Interop::Callback<void()>&& closeHandler)
            {
                msclr::auto_handle<Component> component;

                try
                {
                    component.reset(MakeComponent(std::move(connection), std::move(closeHandler), *m_config));

                    if (auto accessor = m_errorHandler.Component)
                    {
                        accessor->TriggerConnected(component.get());
                    }

                    component->Closed += gcnew System::EventHandler(this, &ComponentFactoryLambdaBase::operator());

                    return{ (void*)true, ComponentHolder<Component^>{ component.release() } };
                }
                catch (const std::exception& /*e*/)
                {
                    m_errorHandler(std::current_exception());
                    return{};
                }
                catch (System::Exception^ e)
                {
                    m_errorHandler(e);
                    return{};
                }
            }

            void operator()(System::Object^ sender, System::EventArgs^ /*args*/)
            {
                if (auto accessor = m_errorHandler.Component)
                {
                    accessor->TriggerDisconnected(safe_cast<Interface^>(sender));
                }
            }

        protected:
            ComponentFactoryLambdaBase(const NativeConfig& config, AccessorBase<Interface^>^ accessor)
                : m_config{ config },
                  m_errorHandler{ accessor }
            {}

            virtual Component^ MakeComponent(
                typename NativeComponent::ConnectionPtr&& connection,
                Interop::Callback<void()>&& closeHandler,
                const NativeConfig& config) = 0;

        private:
            NativeObject<NativeConfig> m_config;
            ErrorHandler<AccessorBase<Interface^>> m_errorHandler;
        };

    } // detail

} // Managed
} // IPC
