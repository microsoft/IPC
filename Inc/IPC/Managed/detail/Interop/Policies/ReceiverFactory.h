#pragma once

#include "IPC/Managed/detail/Interop/Callback.h"
#include <functional>


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
        class ReceiverFactory
        {
            using WorkItemFunc = Callback<void()>;
            using QueueWorkItemFunc = Callback<void(WorkItemFunc&&)>;
            using QueueWorkItemFactoryFunc = std::function<QueueWorkItemFunc()>;

        public:
            ReceiverFactory(const QueueWorkItemFactoryFunc& queueWorkItemFactory);

            template <typename Queue, typename Handler>
            auto operator()(Queue& queue, Handler&& handler) const
            {
                return [queueWorkItem = m_queueWorkItemFactory(), &queue, handler = std::forward<Handler>(handler)]() mutable
                {
                    queueWorkItem(
                        [&queue, &handler]
                        {
                            auto value = queue.Pop();
                            assert(value);

                            handler(std::move(*value));
                        });

                    return 1;
                };
            }

        private:
            QueueWorkItemFactoryFunc m_queueWorkItemFactory;
        };

    } // Policies
    } // Interop
    } // detail
} // Managed
} // IPC
