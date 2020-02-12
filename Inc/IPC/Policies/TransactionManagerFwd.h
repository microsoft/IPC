#pragma once


namespace IPC
{
namespace Policies
{
    class InfiniteTimeoutFactory;

    template <typename Context, typename TimeoutFactory = InfiniteTimeoutFactory>
    class TransactionManager;

} // Policies
} // IPC
