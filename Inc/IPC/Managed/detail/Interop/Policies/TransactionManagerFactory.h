#pragma once

#include "TimeoutFactory.h"
#include <IPC/DefaultTraitsFwd.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
    namespace Policies
    {
        class TransactionManagerFactory
        {
        public:
            TransactionManagerFactory(TimeoutFactory timeoutFactory, const std::chrono::milliseconds& defaultRequestTimeout);

            template <typename T>
            auto operator()(IPC::detail::Identity<IPC::Policies::TransactionManager<T, TimeoutFactory>>) const
            {
                return IPC::Policies::TransactionManager<T, TimeoutFactory>{ m_timeoutFactory, m_defaultRequestTimeout };
            }

        private:
            TimeoutFactory m_timeoutFactory;
            std::chrono::milliseconds m_defaultRequestTimeout;
        };

    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
