#include "stdafx.h"
#include "IPC/detail/ChannelFactory.h"


namespace IPC
{
    namespace detail
    {
        std::shared_ptr<SharedMemory> ChannelFactory<void>::InstanceBase::GetMemory(
            create_only_t, bool input, const char* name, const ChannelSettingsBase& settings)
        {
            return GetMemory(false, input, name, settings);
        }

        std::shared_ptr<SharedMemory> ChannelFactory<void>::InstanceBase::GetMemory(
            open_only_t, bool input, const char* name, const ChannelSettingsBase& settings)
        {
            return GetMemory(true, input, name, settings);
        }

        std::shared_ptr<SharedMemory> ChannelFactory<void>::InstanceBase::GetMemory(
            bool open, bool input, const char* name, const ChannelSettingsBase& settings)
        {
            const auto& config = settings.GetConfig();
            const auto& channelConfig = input ? config.m_input : config.m_output;

            if (config.m_shared)
            {
                if (auto memory = m_current.lock())
                {
                    return memory;
                }
            }

            auto memory = channelConfig.m_common
                ? channelConfig.m_common
                : open
                    ? settings.GetMemoryCache()->Open(name)
                    : settings.GetMemoryCache()->Create(name, channelConfig.m_size);

            if (config.m_shared)
            {
                m_current = memory;
            }

            return memory;
        }

    } // detail
} // IPC
