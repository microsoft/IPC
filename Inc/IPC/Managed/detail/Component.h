#pragma once

#include "NativeObject.h"
#include "CloseHandler.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T, typename IComponent>
        ref class Component : IComponent
        {
        public:
            property SharedMemory^ InputMemory
            {
                virtual SharedMemory^ get() { return %m_inputMemory; }
            }

            property SharedMemory^ OutputMemory
            {
                virtual SharedMemory^ get() { return %m_outputMemory; }
            }

            property System::Boolean IsClosed
            {
                virtual System::Boolean get() { return m_component->IsClosed(); }
            }

            virtual void Close()
            {
                m_component->Close();
            }

            virtual event System::EventHandler^ Closed;

        internal:
            void TriggerClosed()
            {
                Closed(this, System::EventArgs::Empty);
            }

        protected:
            template <typename... Args>
            Component(typename T::ConnectionPtr&& connection, std::nullptr_t, Args&&... args)
                : Component{ std::move(connection), MakeCloseHander(this), false, std::forward<Args>(args)... }
            {}

            template <typename... Args>
            Component(typename T::ConnectionPtr&& connection, Interop::Callback<void()>&& closeHandler, Args&&... args)
                : Component{ std::move(connection), std::move(closeHandler), true, std::forward<Args>(args)... }
            {}

            SharedMemory m_inputMemory;
            SharedMemory m_outputMemory;
            NativeObject<T> m_component;

        private:
            template <typename... Args>
            Component(typename T::ConnectionPtr&& connection, Interop::Callback<void()>&& closeHandler, bool registerClose, Args&&... args)
                : m_inputMemory{ connection.GetInputMemory() },
                  m_outputMemory{ connection.GetOutputMemory() },
                  m_component{ std::move(connection), std::move(closeHandler), std::forward<Args>(args)... }
            {
                if (registerClose)
                {
                    m_component->RegisterCloseHandler(MakeCloseHander(this));
                }
            }
        };

    } // detail

} // Managed
} // IPC
