#pragma once

#include <utility>


namespace IPC
{
namespace Policies
{
    class InlineReceiverFactory
    {
    public:
        template <typename Queue, typename Handler>
        auto operator()(Queue& queue, Handler&& handler) const
        {
            return [&queue, handler = std::forward<Handler>(handler)]() mutable { return queue.ConsumeAll(handler); };
        }
    };

} // Policies
} // IPC
