#pragma once

#include <tuple>
#include <utility>


namespace IPC
{
    namespace detail
    {
        template <typename Function, typename Tuple, std::size_t... I>
        decltype(auto) ApplyTupleHelper(Function&& func, Tuple&& args, std::index_sequence<I...>)
        {
            (void)args;
            return std::forward<Function>(func)(std::get<I>(std::forward<Tuple>(args))...);
        }

        template <typename Function, typename Tuple>
        decltype(auto) ApplyTuple(Function&& func, Tuple&& args)
        {
            return ApplyTupleHelper(
                std::forward<Function>(func),
                std::forward<Tuple>(args),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
        }

    } // detail
} // IPC
