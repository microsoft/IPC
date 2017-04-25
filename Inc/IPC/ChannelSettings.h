#pragma once

#include "detail/ChannelSettingsBase.h"


namespace IPC
{
    struct DefaultTraits;


    template <typename Traits>
    class ChannelSettings : public detail::ChannelSettingsBase
    {
    public:
        ChannelSettings()
            : ChannelSettings{ {} }
        {}

        explicit ChannelSettings(
            typename Traits::WaitHandleFactory waitHandleFactory,
            typename Traits::ReceiverFactory receiverFactory = {},
            std::shared_ptr<SharedMemoryCache> cache = {})
            : ChannelSettingsBase{ std::move(cache) },
              m_waitHandleFactory{ std::move(waitHandleFactory) },
              m_receiverFactory{ std::move(receiverFactory) }
        {}

        const auto& GetWaitHandleFactory() const
        {
            return m_waitHandleFactory;
        }

        const auto& GetReceiverFactory() const
        {
            return m_receiverFactory;
        }

    private:
        typename Traits::WaitHandleFactory m_waitHandleFactory;
        typename Traits::ReceiverFactory m_receiverFactory;
    };


    using DefaultChannelSettings = ChannelSettings<DefaultTraits>;

} // IPC
