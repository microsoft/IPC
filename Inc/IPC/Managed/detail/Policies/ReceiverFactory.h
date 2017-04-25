#pragma once

#pragma managed(push, off)
#include "IPC/Managed/detail/Interop/Policies/ReceiverFactory.h"
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
        IPC_MANAGED_DLL Interop::Policies::ReceiverFactory MakeReceiverFactory();

    } // Policies
    } // detail

} // Managed
} // IPC
