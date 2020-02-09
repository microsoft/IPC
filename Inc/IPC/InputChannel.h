#pragma once

#include "detail/ChannelBase.h"
#include <memory>
#include <stdexcept>


namespace IPC
{
    struct DefaultTraits;


    template <typename T, typename Traits = DefaultTraits>
    class InputChannel : public detail::ChannelBase<T, Traits::template Queue>
    {
        using ChannelBase = detail::ChannelBase<T, Traits::template Queue>;
        using WaitHandleFactory = typename Traits::WaitHandleFactory;
        using ReceiverFactory = typename Traits::ReceiverFactory;

    public:
        InputChannel(create_only_t, const char* name, std::shared_ptr<SharedMemory> memory, WaitHandleFactory waitHandleFactory = {}, ReceiverFactory receiverFactory = {})
            : InputChannel{ ChannelBase{ create_only, name, std::move(memory) }, std::move(waitHandleFactory), std::move(receiverFactory) }
        {}

        InputChannel(open_only_t, const char* name, std::shared_ptr<SharedMemory> memory, WaitHandleFactory waitHandleFactory = {}, ReceiverFactory receiverFactory = {})
            : InputChannel{ ChannelBase{ open_only, name, std::move(memory) }, std::move(waitHandleFactory), std::move(receiverFactory) }
        {}

        template <typename Handler>
        std::size_t ReceiveAll(Handler&& handler)
        {
            return ConsumeAll([&] { return this->GetQueue().ConsumeAll(handler); });
        }

        template <typename Handler>
        bool RegisterReceiver(Handler&& handler)
        {
            if (m_notEmptyMonitor)
            {
                return false;
            }

            m_notEmptyMonitor = m_waitHandleFactory(
                this->GetNotEmptyEvent(),
                [this, receiver = m_receiverFactory(this->GetQueue(), std::forward<Handler>(handler))]() mutable
                {
                    ConsumeAll(receiver);
                    return true;
                });

            return true;
        }

        bool UnregisterReceiver()
        {
            if (!m_notEmptyMonitor)
            {
                return false;
            }

            m_notEmptyMonitor = {};

            return true;
        }

    private:
        InputChannel(ChannelBase&& base, WaitHandleFactory&& waitHandleFactory, ReceiverFactory&& receiverFactory)
            : ChannelBase{ std::move(base) },
              m_waitHandleFactory{ std::move(waitHandleFactory) },
              m_receiverFactory{ std::move(receiverFactory) }
        {}

        template <typename Receiver>
        std::size_t ConsumeAll(Receiver&& receiver)
        {
            if (auto& counter = this->GetCounter())
            {
                auto memory = this->GetMemory();    // Make sure memory does not die if receiver deletes this.
                std::size_t n, count{ 0 };

                do
                {
                    count += (n = receiver());      // Receiver may delete this.

                } while ((counter -= n) != 0);

                return count;
            }

            return 0;
        }

        using WaitHandle = decltype(std::declval<WaitHandleFactory>()(
            std::declval<detail::KernelObject>(), std::declval<bool(*)()>()));

        WaitHandleFactory m_waitHandleFactory;
        WaitHandle m_notEmptyMonitor;
        ReceiverFactory m_receiverFactory;
    };

} // IPC
