#pragma once


namespace IPC
{
namespace Policies
{
    class TransactionManagerFactory
    {
    public:
        template <typename T>
        auto operator()(T&&) const
        {
            return typename std::decay_t<T>::type{};
        }
    };

} // Policies
} // IPC
