#pragma once

#include <type_traits>


namespace IPC
{
    namespace detail
    {
        template <typename T, typename Id>
        class RequestPacket;

        template <typename T, typename Id>
        class ResponsePacket;


        template <typename Request, typename Response, typename Traits>
        struct PacketInfo
        {
            using RequestPacket = RequestPacket<Request, typename Traits::PacketId>;
            using ResponsePacket = ResponsePacket<Response, typename Traits::PacketId>;
        };


        template <typename Request, typename Traits>
        struct PacketInfo<Request, void, Traits>
        {
            using RequestPacket = RequestPacket<Request, void>;
            using ResponsePacket = void;
        };


        template <typename Response, typename Traits>
        struct PacketInfo<void, Response, Traits>
        {
            static_assert(!std::is_same<Response, Response>::value, "Request cannot be void.");
        };


        template <typename RequestT, typename ResponseT, typename TraitsT>
        struct ClientPacketInfo
        {
            using Request = RequestT;
            using Response = ResponseT;
            using Traits = TraitsT;

            using InputPacket = typename PacketInfo<Request, Response, Traits>::ResponsePacket;
            using OutputPacket = typename PacketInfo<Request, Response, Traits>::RequestPacket;
        };

        template <typename Request, typename Response, typename Traits>
        struct ServerPacketInfo : ClientPacketInfo<Request, Response, Traits>
        {
            using InputPacket = typename PacketInfo<Request, Response, Traits>::RequestPacket;
            using OutputPacket = typename PacketInfo<Request, Response, Traits>::ResponsePacket;
        };

    } // detail
} // IPC
