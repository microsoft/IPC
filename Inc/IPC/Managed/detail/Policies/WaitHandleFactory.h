#pragma once

#pragma managed(push, off)
#include "IPC/Managed/detail/Interop/Policies/WaitHandleFactory.h"
#pragma managed(on)

#include "IPC/Managed/detail/Export.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Policies
    {
        IPC_MANAGED_DLL Interop::Policies::WaitHandleFactory MakeWaitHandleFactory();

    } // Policies
    } // detail

} // Managed
} // IPC
