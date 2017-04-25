#pragma once

#include "ThreadPool.h"
#include "IPC/detail/Callback.h"
#include "IPC/detail/KernelObject.h"
#include <memory>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
namespace Policies
{
    class WaitHandleFactory
    {
    public:
        WaitHandleFactory() = default;

        explicit WaitHandleFactory(boost::optional<ThreadPool> pool);

        std::shared_ptr<void> operator()(detail::KernelObject obj, detail::Callback<bool()> handler) const;

    private:
        class Waiter;

        boost::optional<ThreadPool> m_pool;
    };

} // Policies
} // IPC
