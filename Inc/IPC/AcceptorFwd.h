#pragma once

#include "detail/PacketFwd.h"


namespace IPC
{
    struct DefaultTraits;

    template <typename Input, typename Output, typename Traits = DefaultTraits>
    class Acceptor;

    template <typename PacketInfo>
    class PacketAcceptor;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ClientAcceptor = PacketAcceptor<detail::ClientPacketInfo<Request, Response, Traits>>;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ServerAcceptor = PacketAcceptor<detail::ServerPacketInfo<Request, Response, Traits>>;

} // IPC
