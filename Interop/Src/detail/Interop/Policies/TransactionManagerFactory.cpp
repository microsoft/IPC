#include "stdafx.h"
#include "IPC/Managed/detail/Interop/Policies/TransactionManagerFactory.h"


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
        TransactionManagerFactory::TransactionManagerFactory(TimeoutFactory timeoutFactory, const std::chrono::milliseconds& defaultRequestTimeout)
            : m_timeoutFactory{ std::move(timeoutFactory) },
              m_defaultRequestTimeout{ defaultRequestTimeout }
        {}

    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
