#pragma once

#include "IPC/DefaultTraits.h"
#include "IPC/Policies/InfiniteTimeoutFactory.h"
#include "WaitHandleFactoryMock.h"
#include <exception>


namespace IPC
{
namespace UnitTest
{
namespace Mocks
{
    struct NullTimeoutTraits : DefaultTraits
    {
        using TimeoutFactory = Policies::InfiniteTimeoutFactory;

        template <typename Context>
        using TransactionManager = Policies::TransactionManager<Context, TimeoutFactory>;
    };

    struct Traits : NullTimeoutTraits
    {
        using WaitHandleFactory = Mocks::WaitHandleFactory;

        using ReceiverFactory = Policies::InlineReceiverFactory;

        struct ErrorHandler
        {
            [[noreturn]] void operator()(std::exception_ptr e) const
            {
                std::rethrow_exception(e);
            }
        };
    };


} // Mocks
} // UnitTest
} // IPC
