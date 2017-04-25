#pragma once
#pragma managed(push, off)

#include <type_traits>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template <typename T>
        struct ExternalConstructor : std::false_type
        {};

    } // Interop
    } // detail

} // Managed
} // IPC

#pragma managed(pop)
