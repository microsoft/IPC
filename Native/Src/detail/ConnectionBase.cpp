#include "stdafx.h"
#include "IPC/detail/ConnectionBase.h"
#include "IPC/Exception.h"
#include <cassert>


namespace IPC
{
    namespace detail
    {
        ConnectionBase::ConnectionBase(KernelEvent localCloseEvent)
            : m_closeEvent{ std::move(localCloseEvent) }
        {}

#ifdef NDEBUG
        ConnectionBase::~ConnectionBase() = default;
#else
        ConnectionBase::~ConnectionBase()
        {
            assert(IsClosed());
        }
#endif

        bool ConnectionBase::IsClosed() const
        {
            return m_isClosed;
        }

        void ConnectionBase::Close()
        {
            if (m_closeEvent)
            {
                m_closeEvent.Signal();
            }

            CloseHandler(); // Be prepared for self destruction.
        }

        bool ConnectionBase::RegisterCloseHandler(Callback<void()> handler, bool combine)
        {
            {
                std::lock_guard<std::mutex> lock{ m_closeHandlerMutex };

                if (!m_closeHandler)
                {
                    m_closeHandler = std::move(handler);
                }
                else if (combine)
                {
                    auto newHandler = [closeHandler = std::move(m_closeHandler), handler = std::move(handler)]() mutable
                        {
                            closeHandler();
                            handler();
                        };

                    m_closeHandler = std::move(newHandler);
                }
                else
                {
                    return false;
                }
            }

            if (IsClosed())
            {
                CloseHandler(); // Be prepared for self destruction.
            }

            return true;
        }

        bool ConnectionBase::UnregisterCloseHandler()
        {
            std::lock_guard<std::mutex> lock{ m_closeHandlerMutex };

            if (!m_closeHandler)
            {
                return false;
            }

            m_closeHandler = {};

            return true;
        }

        void ConnectionBase::ThrowIfClosed() const
        {
            if (IsClosed())
            {
                throw Exception{ "Connection is closed." };
            }
        }

        void ConnectionBase::CloseHandler()
        {
            m_isClosed = true;

            Callback<void()> handler;
            {
                std::lock_guard<std::mutex> lock{ m_closeHandlerMutex };
                handler = std::move(m_closeHandler);    // Make sure the handler is invoked only once.
            }

            if (handler)
            {
                handler();  // Be prepared for self destruction.
            }
        }

    } // detail
} // IPC
