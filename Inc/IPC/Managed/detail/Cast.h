#pragma once

#pragma managed(push, off)
#include <type_traits>
#pragma managed(pop)


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T, typename Enable = void>
        struct Convert
        {
            static_assert(!std::is_same<T, T>::value, "No conversion available for this type.");
        };

        template <typename T>
        using CastType = typename Convert<T>::type;

        template <typename T, typename = std::enable_if_t<!__is_class(T)>>
        decltype(auto) Cast(T% value)
        {
            return Convert<CastType<T>>::From(value);
        }

        template <typename T, typename = std::enable_if_t<__is_class(T)>>
        CastType<std::decay_t<T>> Cast(const T& value)
        {
            return Convert<CastType<std::decay_t<T>>>::From(value);
        }

    } // detail

} // Managed
} // IPC
