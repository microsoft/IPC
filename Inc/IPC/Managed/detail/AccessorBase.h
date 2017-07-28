#pragma once

#include "NativeObject.h"
#include "ComponentHolder.h"
#include "ErrorHandler.h"
#include "Interop/Callback.h"

#pragma managed(push, off)
#include <boost/optional.hpp>
#pragma managed(pop)

#include <msclr/auto_handle.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T, typename NativeAccessor>
        ref class AccessorBase abstract : IAccessor<T>
        {
        public:
            virtual event System::EventHandler<ComponentEventArgs<T>^>^ Connected;

            virtual event System::EventHandler<ComponentEventArgs<T>^>^ Disconnected;

            virtual event System::EventHandler<ErrorEventArgs^>^ Error;

            virtual property System::Boolean Enabled
            {
                System::Boolean get()
                {
                    return static_cast<bool>(*m_accessor);
                }

                void set(System::Boolean value)
                {
                    if (Enabled != value)
                    {
                        try
                        {
                            *m_accessor = value ? boost::optional<NativeAccessor>{ MakeAccessor() } : boost::none;
                        }
                        catch (const std::exception& /*e*/)
                        {
                            ThrowManagedException(std::current_exception());
                        }
                    }
                }
            }

        protected:
            AccessorBase()
            {}

            virtual NativeAccessor MakeAccessor() abstract;

            property NativeAccessor& Accessor
            {
                NativeAccessor& get()
                {
                    if (!Enabled)
                    {
                        throw gcnew Exception{ "Accessor is disabled." };
                    }

                    return **m_accessor;
                }
            }

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

        private:
            NativeObject<boost::optional<NativeAccessor>> m_accessor;
        };


        template <typename NativeComponent, typename NativeConfig, typename NativeAccessor, typename Component, typename Interface>
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
            ComponentFactoryLambdaBase(const NativeConfig& config, AccessorBase<Interface^, NativeAccessor>^ accessor)
                : m_config{ config },
                  m_errorHandler{ accessor }
            {}

            virtual Component^ MakeComponent(
                typename NativeComponent::ConnectionPtr&& connection,
                Interop::Callback<void()>&& closeHandler,
                const NativeConfig& config) abstract;

        private:
            NativeObject<NativeConfig> m_config;
            ErrorHandler<AccessorBase<Interface^, NativeAccessor>> m_errorHandler;
        };

    } // detail

} // Managed
} // IPC
