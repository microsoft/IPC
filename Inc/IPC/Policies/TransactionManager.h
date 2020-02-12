#pragma once

#include "TransactionManagerFwd.h"
#include "InfiniteTimeoutFactory.h"
#include "IPC/detail/LockFree/IndexedObjectPool.h"
#include <chrono>
#include <cassert>

#pragma warning(push)
#include <boost/optional.hpp>
#pragma warning(pop)


namespace IPC
{
namespace Policies
{
    template <typename Context, typename TimeoutFactory>
    class TransactionManager
    {
    public:
        using Id = std::uint32_t;

        TransactionManager() = default;

        explicit TransactionManager(TimeoutFactory timeoutFactory, const std::chrono::milliseconds& defaultTimeout = {})
            : m_timeoutFactory{ std::move(timeoutFactory) },
              m_defaultTimeout{ NonZeroTimeout(defaultTimeout) }
        {}

        template <typename OtherContext>
        Id BeginTransaction(OtherContext&& context, const std::chrono::milliseconds& timeout = {})
        {
            auto result = m_transactions->Take(
                [this](Id id)
                {
                    return m_timeoutFactory([this, id] { EndTransaction(id); });
                });

            Transaction& transaction = result.first;
            Id id = result.second;

            try
            {
                transaction.Begin(std::forward<OtherContext>(context), NonZeroTimeout(timeout, m_defaultTimeout));
            }
            catch (...)
            {
                m_transactions->Return(id);
                throw;
            }

            return id;
        }

        boost::optional<Context> EndTransaction(Id id)
        {
            boost::optional<Context> context;

            m_transactions->Return(id, [&](auto& transaction) { context = transaction.End(); });

            return context;
        }

        void TerminateTransactions()
        {
            m_transactions->ReturnAll([](auto& transaction) { transaction.End(); });
        }

        const std::chrono::milliseconds& GetDefaultTimeout() const
        {
            return m_defaultTimeout;
        }

    private:
        class Transaction
        {
        public:
            template <typename SchedulerFactory>
            Transaction(Id id, SchedulerFactory&& schedulerFactory)
                : m_timeoutScheduler{ std::forward<SchedulerFactory>(schedulerFactory)(id) }
            {}

            template <typename OtherContext>
            void Begin(OtherContext&& context, const std::chrono::milliseconds& timeout)
            {
                assert(timeout != std::chrono::milliseconds::zero());
                m_context = std::forward<OtherContext>(context);
                m_timeoutScheduler(timeout);
            }

            boost::optional<Context> End()
            {
                m_timeoutScheduler(nullptr);    // This must wait for callback (which effectively runs End) to complete.

                decltype(m_context) context;
                context.swap(m_context);
                return context;
            }

        private:
            using TimeoutScheduler = decltype(std::declval<TimeoutFactory>()({}));

            boost::optional<Context> m_context;
            TimeoutScheduler m_timeoutScheduler;
        };

        using TransactionPool = detail::LockFree::IndexedObjectPool<Transaction, std::allocator<void>>;

        static_assert(std::is_same<Id, typename TransactionPool::Index>::value, "Id and Index must have the same type.");

        static constexpr auto NonZeroTimeout()
        {
            return std::chrono::seconds{ 3 };
        }

        template <typename Rep, typename Period, typename... Timeouts>
        static constexpr decltype(auto) NonZeroTimeout(const std::chrono::duration<Rep, Period>& timeout, Timeouts&&... timeouts)
        {
            return timeout != std::chrono::duration<Rep, Period>::zero() ? timeout : NonZeroTimeout(std::forward<Timeouts>(timeouts)...);
        }


        std::unique_ptr<TransactionPool> m_transactions{ std::make_unique<TransactionPool>() };
        TimeoutFactory m_timeoutFactory;
        std::chrono::milliseconds m_defaultTimeout{ NonZeroTimeout() };
    };

} // Policies
} // IPC
