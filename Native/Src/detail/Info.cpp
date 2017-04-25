#include "stdafx.h"
#include "IPC/detail/Info.h"


namespace IPC
{
    namespace detail
    {
        AcceptorHostInfo::AcceptorHostInfo(const String::allocator_type& allocator)
            : m_acceptorCloseEventName{ allocator }
        {}

        ConnectorPingInfo::ConnectorPingInfo(const String::allocator_type& allocator)
            : m_connectorCloseEventName{ allocator },
              m_connectorInfoChannelName{ allocator },
              m_acceptorInfoChannelName{ allocator }
        {}

        ConnectorInfo::ConnectorInfo(const String::allocator_type& allocator)
            : m_channelName{ allocator }
        {}

        AcceptorInfo::AcceptorInfo(const String::allocator_type& allocator)
            : m_closeEventName{ allocator },
              m_channelName{ allocator }
        {}

    } // detail
} // IPC
