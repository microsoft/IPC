#pragma once

#pragma warning(push)
#include <boost/function.hpp>
#pragma warning(pop)

#include <type_traits>
#include <cassert>


namespace IPC
{
    namespace detail
    {
        template <typename Function>
        class CopyableFunction
        {
        public:
            explicit CopyableFunction(Function&& func)
                : m_func{ std::move(func) }
            {}

            CopyableFunction(const CopyableFunction& other)
                : CopyableFunction{ std::move(const_cast<CopyableFunction&>(other)) }
            {}

            CopyableFunction(CopyableFunction&& other) = default;

            template <typename... Args>
            decltype(auto) operator()(Args&&... args)
            {
                return m_func(std::forward<Args>(args)...);
            }

            template <typename... Args>
            decltype(auto) operator()(Args&&... args) const
            {
                return m_func(std::forward<Args>(args)...);
            }

        private:
            Function m_func;
        };


        template <typename Function>
        class Callback;


        template <typename Result, typename... Args>
        class Callback<Result(Args...)>
        {
        public:
            Callback() = default;

            template <typename Function, typename = std::enable_if_t<!std::is_base_of<Callback, std::remove_reference_t<Function>>::value>,
                std::enable_if_t<!std::is_copy_constructible<std::decay_t<Function>>::value>* = nullptr>
            Callback(Function&& func)
                : m_func{ CopyableFunction<std::decay_t<Function>>{ std::forward<Function>(func) } }
            {}

            template <typename Function, typename = std::enable_if_t<!std::is_base_of<Callback, std::remove_reference_t<Function>>::value>,
                std::enable_if_t<std::is_copy_constructible<std::decay_t<Function>>::value>* = nullptr>
            Callback(Function&& func)
                : m_func{ std::forward<Function>(func) }
            {}

            Callback(const Callback& other) = delete;
            Callback& operator=(const Callback& other) = delete;

            Callback(Callback&& other) = default;
            Callback& operator=(Callback&& other) = default;

            explicit operator bool() const
            {
                return static_cast<bool>(m_func);
            }

            template <typename... OtherArgs>
            decltype(auto) operator()(OtherArgs&&... args) const
            {
                assert(m_func);
                return m_func(std::forward<OtherArgs>(args)...);
            }

        private:
            boost::function<Result(Args...)> m_func;    // TODO: Use std::function when bugs are fixed in VC14 (capturing nested polymorphic lambdas fails).
        };

    } // detail
} // IPC
