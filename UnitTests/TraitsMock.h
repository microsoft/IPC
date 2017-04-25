#pragma once

#include "IPC/DefaultTraits.h"
#include "IPC/Policies/NullTimeoutFactory.h"
#include "WaitHandleFactoryMock.h"
#include <exception>


namespace IPC
{
namespace UnitTest
{
namespace Mocks
{
    struct Traits : DefaultTraits
    {
        using WaitHandleFactory = Mocks::WaitHandleFactory;

        using ReceiverFactory = Policies::InlineReceiverFactory;

        using TimeoutFactory = Policies::NullTimeoutFactory;

        template <typename Context>
        using TransactionManager = Policies::TransactionManager<Context, TimeoutFactory>;

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
