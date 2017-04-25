#pragma once

#include "detail/LockFree/QueueFwd.h"
#include "Policies/ReceiverFactoryFwd.h"
#include "Policies/TransactionManagerFwd.h"
#include <cstdint>
#include <cstddef>


namespace IPC
{
    namespace detail
    {
        template <typename T>
        struct Identity
        {
            using type = T;
        };

    } // detail


    namespace Policies
    {
        class WaitHandleFactory;

        class TimeoutFactory;

        class TransactionManagerFactory;

        class ErrorHandler;

    } // Policies


    struct DefaultTraits
    {
        template <typename T, typename Allocator>
        using Queue = detail::LockFree::Queue<T, Allocator>;

        using WaitHandleFactory = Policies::WaitHandleFactory;

        using ReceiverFactory = Policies::ReceiverFactory;

        using TimeoutFactory = Policies::TimeoutFactory;

        template <typename Context>
        using TransactionManager = Policies::TransactionManager<Context, TimeoutFactory>;

        using TransactionManagerFactory = Policies::TransactionManagerFactory;

        using PacketId = std::uint32_t;

        using ErrorHandler = Policies::ErrorHandler;
    };

} // IPC
