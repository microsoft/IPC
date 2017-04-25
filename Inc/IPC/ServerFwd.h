#pragma once

#include "detail/PacketConnectionFwd.h"
#include "detail/PacketFwd.h"


namespace IPC
{
    struct DefaultTraits;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    class Server;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ServerConnection = detail::PacketConnection<detail::ServerPacketInfo<Request, Response, Traits>>;

} // IPC
