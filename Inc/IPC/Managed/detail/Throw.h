#pragma once

#ifdef __cplusplus_cli
#pragma managed(push, off)
#endif

#include "Export.h"
#include <exception>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        [[noreturn]] IPC_MANAGED_DLL void ThrowManagedException(std::exception_ptr error);

        template <typename Function, typename... Args>
        decltype(auto) InvokeThrow(Function&& func, Args&&... args)
        {
            try
            {
                return std::forward<Function>(func)(std::forward<Args>(args)...);
            }
            catch (const std::exception& /*e*/)
            {
                ThrowManagedException(std::current_exception());
            }
        }

    } // detail

} // Managed
} // IPC

#ifdef __cplusplus_cli
#pragma managed(pop)
#endif
