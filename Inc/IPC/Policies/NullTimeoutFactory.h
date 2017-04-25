#pragma once

#include "IPC/detail/Callback.h"
#include <chrono>


namespace IPC
{
namespace Policies
{
    /// Provides infinite timeout behavior. The provided handler is not preserved.
    class NullTimeoutFactory
    {
    public:
        NullTimeoutFactory(const std::chrono::milliseconds& /*defaultTimeout*/ = {})
        {}

        auto operator()(const detail::Callback<void()>& /*handler*/) const
        {
            return [](auto&&...) {};
        }
    };

} // Policies
} // IPC
