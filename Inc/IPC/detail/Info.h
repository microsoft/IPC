#pragma once

#include "Alias.h"
#include "IPC/SharedMemory.h"

#pragma warning(push)
#include <boost/interprocess/containers/string.hpp>
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
        using String = ipc::basic_string<char, std::char_traits<char>, SharedMemory::Allocator<char>>;

        struct Settings
        {
            boost::optional<String> m_commonInputMemoryName;
            boost::optional<String> m_commonOutputMemoryName;
        };

        struct AcceptorHostInfo
        {
            explicit AcceptorHostInfo(const String::allocator_type& allocator);

            std::uint32_t m_processId{};
            String m_acceptorCloseEventName;

            Settings m_settings;

            // TODO: Add compatibility version info.
        };

        struct ConnectorPingInfo
        {
            explicit ConnectorPingInfo(const String::allocator_type& allocator);

            std::uint32_t m_processId{};
            String m_connectorCloseEventName;
            String m_connectorInfoChannelName;
            String m_acceptorInfoChannelName;

            Settings m_settings;
        };

        struct ConnectorInfo
        {
            explicit ConnectorInfo(const String::allocator_type& allocator);

            String m_channelName;
        };

        struct AcceptorInfo
        {
            explicit AcceptorInfo(const String::allocator_type& allocator);

            String m_closeEventName;
            String m_channelName;
        };

    } // detail
} // IPC
