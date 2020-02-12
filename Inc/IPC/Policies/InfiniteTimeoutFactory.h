#pragma once

#include "IPC/detail/Callback.h"
#include <chrono>


namespace IPC
{
namespace Policies
{
    /// Provides infinite timeout behavior. The provided handler is not preserved.
    class InfiniteTimeoutFactory
    {
    public:
        InfiniteTimeoutFactory(const std::chrono::milliseconds& /*defaultTimeout*/ = {})
        {}

        auto operator()(const detail::Callback<void()>& /*handler*/) const
        {
            return [](auto&&...) {};
        }
    };

} // Policies
} // IPC
