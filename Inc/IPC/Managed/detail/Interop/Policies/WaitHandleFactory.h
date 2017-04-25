#pragma once

#include "IPC/detail/KernelObject.h"
#include "IPC/Managed/detail/Interop/Callback.h"
#include <functional>
#include <memory>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
    namespace Policies
    {
        using WaitHandleFactory = std::function<std::shared_ptr<void>(IPC::detail::KernelObject, Callback<bool()>)>;


    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
