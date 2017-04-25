#pragma once

#include "detail/PacketConnectionFwd.h"
#include "detail/PacketFwd.h"


namespace IPC
{
    struct DefaultTraits;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    class Client;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ClientConnection = detail::PacketConnection<detail::ClientPacketInfo<Request, Response, Traits>>;

} // IPC
