#pragma once

#include "Cast.h"

#pragma managed(push, off)
#include <type_traits>
#pragma managed(pop)


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T>
        struct Convert<T, std::enable_if_t<std::is_arithmetic<T>::value>>
        {
            using type = T;

            static type From(T% from)
            {
                return{ from };
            }
        };

    } // detail

} // Managed
} // IPC
