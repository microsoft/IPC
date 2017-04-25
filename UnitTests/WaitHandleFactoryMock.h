#pragma once

#include "IPC/detail/KernelObject.h"
#include "IPC/detail/Callback.h"
#include <memory>
#include <map>
#include <random>


namespace IPC
{
namespace UnitTest
{
namespace Mocks
{
    struct KernelObjectComparer
    {
        bool operator()(const detail::KernelObject& left, const detail::KernelObject& right) const
        {
            return static_cast<void*>(left) < static_cast<void*>(right);
        }
    };

    class WaitHandleFactory
        : public std::shared_ptr<std::map<detail::KernelObject, std::weak_ptr<detail::Callback<bool()>>, KernelObjectComparer>>
    {
    public:
        WaitHandleFactory();

        std::size_t Process();

        std::shared_ptr<void> operator()(detail::KernelObject obj, detail::Callback<bool()> handler) const;

    private:
        std::size_t ProcessSinglePass();

        std::shared_ptr<std::random_device> m_rd;
    };


} // Mocks
} // UnitTest
} // IPC
