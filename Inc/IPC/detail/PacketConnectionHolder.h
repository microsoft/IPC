#pragma once

#include "ConnectionHolder.h"
#include <memory>
#include <type_traits>


namespace IPC
{
    namespace detail
    {
        template <typename PacketTraits>
        class PacketConnectionHolder
            : public ConnectionHolder<typename PacketTraits::InputPacket, typename PacketTraits::OutputPacket, typename PacketTraits::Traits>,
              protected PacketTraits,
              protected PacketTraits::Context
        {
            using Context = typename PacketTraits::Context;

        public:
            ~PacketConnectionHolder()
            {
                this->GetConnection().Close();
            }

        protected:
            template <typename... Handlers>
            PacketConnectionHolder(std::unique_ptr<typename PacketConnectionHolder::Connection> connection, Context context, Handlers&&... handlers)
                : ConnectionHolder{ std::move(connection) },
                  Context{ std::move(context) }
            {
                this->Register(std::forward<Handlers>(handlers)...);
            }

            const Context& GetContext() const
            {
                return *this;
            }

            Context& GetContext()
            {
                return *this;
            }
        };

    } // detail
} // IPC
