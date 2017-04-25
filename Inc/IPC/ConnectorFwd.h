#pragma once

#include "detail/PacketFwd.h"


namespace IPC
{
    struct DefaultTraits;

    template <typename Input, typename Output, typename Traits = DefaultTraits>
    class Connector;

    template <typename PacketInfoT>
    class PacketConnector;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ClientConnector = PacketConnector<detail::ClientPacketInfo<Request, Response, Traits>>;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ServerConnector = PacketConnector<detail::ServerPacketInfo<Request, Response, Traits>>;

} // IPC
