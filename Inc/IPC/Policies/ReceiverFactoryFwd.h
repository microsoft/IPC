#pragma once


namespace IPC
{
    namespace detail
    {
        template <typename... Factories>
        class VariantReceiverFactory;

    } // detail

namespace Policies
{
    class AsyncReceiverFactory;
    class InlineReceiverFactory;

    using ReceiverFactory = detail::VariantReceiverFactory<AsyncReceiverFactory, InlineReceiverFactory>;

} // Policies
} // IPC
