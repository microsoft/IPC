#pragma once

#include "IPC/ConnectionFwd.h"


namespace IPC
{
    namespace detail
    {
        template <typename PacketTraits>
        using PacketConnection = Connection<typename PacketTraits::InputPacket, typename PacketTraits::OutputPacket, typename PacketTraits::Traits>;

    } // detail

} // IPC
