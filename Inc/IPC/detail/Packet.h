#pragma once

#include "PacketFwd.h"

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
        template <typename T>
        class RequestPacket<T, void>
        {
        public:
            RequestPacket() = default;

            RequestPacket(const T& payload)
                : m_payload{ payload }
            {}

            RequestPacket(T&& payload)
                : m_payload{ std::move(payload) }
            {}

            T& GetPayload()
            {
                return m_payload;
            }

        private:
            T m_payload{};
        };


        template <typename T, typename Id>
        class RequestPacket : public RequestPacket<T, void>
        {
            using Base = RequestPacket<T, void>;

        public:
            RequestPacket() = default;

            template <typename U>
            RequestPacket(const Id& id, U&& payload)
                : Base{ std::forward<U>(payload) },
                  m_id{ id }
            {}

            const Id& GetId() const
            {
                return m_id;
            }

        private:
            Id m_id{};
        };


        template <typename T, typename Id>
        class ResponsePacket
        {
        public:
            ResponsePacket() = default;

            template <typename U>
            ResponsePacket(const Id& id, U&& payload)
                : m_id{ id },
                  m_payload{ std::forward<U>(payload) }
            {}

            ResponsePacket(const Id& id)
                : m_id{ id }
            {}

            const Id& GetId() const
            {
                return m_id;
            }

            bool IsValid() const
            {
                return static_cast<bool>(m_payload);
            }

            T& GetPayload()
            {
                assert(IsValid());
                return *m_payload;
            }

        private:
            Id m_id{};
            boost::optional<T> m_payload;
        };

    } // detail
} // IPC
