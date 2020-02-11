#pragma once

#include "IPC/Connection.h"
#include "Exception.h"
#include <memory>
#include <type_traits>


namespace IPC
{
    namespace detail
    {
        template <typename Input, typename Output, typename Traits>
        class ConnectionHolder
        {
        public:
            using Connection = Connection<Input, Output, Traits>;

            explicit ConnectionHolder(std::unique_ptr<Connection> connection)
                : m_connection{ std::move(connection) }
            {}

            ConnectionHolder(const ConnectionHolder& other) = delete;
            ConnectionHolder& operator=(const ConnectionHolder& other) = delete;

            virtual ~ConnectionHolder() = default;

            Connection& GetConnection()
            {
                return *m_connection;
            }

            const Connection& GetConnection() const
            {
                return *m_connection;
            }

        protected:
            template <typename I = Input, typename CloseHandler, std::enable_if_t<std::is_void<I>::value>* = nullptr>
            void Register(CloseHandler&& closeHandler)
            {
                if (!this->m_connection->RegisterCloseHandler(std::forward<CloseHandler>(closeHandler), true))
                {
                    throw Exception{ "Failed to register for close event." };
                }
            }

            template <typename I = Input, typename Handler, typename CloseHandler, std::enable_if_t<!std::is_void<I>::value>* = nullptr>
            void Register(Handler&& handler, CloseHandler&& closeHandler)
            {
                if (!m_connection->GetInputChannel().RegisterReceiver(std::forward<Handler>(handler)))
                {
                    throw Exception{ "Failed to register a receiver." };
                }

                Register<void>(std::forward<CloseHandler>(closeHandler));
            }

            std::shared_ptr<Connection> GetSharedConnection() const
            {
                return m_connection;
            }

        private:
            std::shared_ptr<Connection> m_connection;
        };

    } // detail
} // IPC
