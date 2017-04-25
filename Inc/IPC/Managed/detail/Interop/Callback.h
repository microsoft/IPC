#pragma once

#ifdef __cplusplus_cli
#pragma managed(push, off)
#endif

#include "IPC/detail/Callback.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        using IPC::detail::Callback;

    } // Interop
    } // detail

} // Managed
} // IPC

#ifdef __cplusplus_cli
#pragma managed(pop)
#endif
