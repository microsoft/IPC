#pragma once


namespace IPC
{
namespace Policies
{
    class NullTimeoutFactory;

    template <typename Context, typename TimeoutFactory = NullTimeoutFactory>
    class TransactionManager;

} // Policies
} // IPC
