#include "stdafx.h"
#include "IPC/Managed/detail/Interop/Policies/ReceiverFactory.h"


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
        ReceiverFactory::ReceiverFactory(const QueueWorkItemFactoryFunc& queueWorkItemFactory)
            : m_queueWorkItemFactory{ queueWorkItemFactory }
        {}

    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
