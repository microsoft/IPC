#include "stdafx.h"
#include "IPC/detail/ChannelSettingsBase.h"
#include <cassert>


namespace IPC
{
    namespace detail
    {
        ChannelSettingsBase::ChannelSettingsBase(std::shared_ptr<SharedMemoryCache> cache)
            : m_cache{ cache ? std::move(cache) : std::make_shared<SharedMemoryCache>() }
        {}

        void ChannelSettingsBase::SetInput(std::size_t memorySize, bool allowOverride)
        {
            m_config.m_shared = false;

            auto& input = m_config.m_input;
            input.m_common = {};
            input.m_size = memorySize;
            input.m_allowOverride = allowOverride;
        }

        void ChannelSettingsBase::SetOutput(std::size_t memorySize, bool allowOverride)
        {
            m_config.m_shared = false;

            auto& output = m_config.m_output;
            output.m_common = {};
            output.m_size = memorySize;
            output.m_allowOverride = allowOverride;
        }

        void ChannelSettingsBase::SetInput(std::shared_ptr<SharedMemory> memory, bool allowOverride)
        {
            assert(memory);

            m_config.m_shared = false;

            auto& input = m_config.m_input;
            input.m_common = std::move(memory);
            input.m_size = 0;
            input.m_allowOverride = allowOverride;
        }

        void ChannelSettingsBase::SetOutput(std::shared_ptr<SharedMemory> memory, bool allowOverride)
        {
            assert(memory);

            m_config.m_shared = false;

            auto& output = m_config.m_output;
            output.m_common = std::move(memory);
            output.m_size = 0;
            output.m_allowOverride = allowOverride;
        }

        void ChannelSettingsBase::SetInputOutput(std::size_t memorySize, bool allowOverride)
        {
            m_config.m_shared = true;

            auto& input = m_config.m_input;
            input.m_common = {};
            input.m_size = memorySize;
            input.m_allowOverride = allowOverride;

            m_config.m_output = input;
        }

        void ChannelSettingsBase::SetInputOutput(std::shared_ptr<SharedMemory> memory, bool allowOverride)
        {
            assert(memory);

            m_config.m_shared = true;

            auto& input = m_config.m_input;
            input.m_common = std::move(memory);
            input.m_size = 0;
            input.m_allowOverride = allowOverride;

            m_config.m_output = input;
        }

        const std::shared_ptr<SharedMemory>& ChannelSettingsBase::GetCommonInput() const
        {
            return m_config.m_input.m_common;
        }

        const std::shared_ptr<SharedMemory>& ChannelSettingsBase::GetCommonOutput() const
        {
            return m_config.m_output.m_common;
        }

        bool ChannelSettingsBase::IsInputOverrideAllowed() const
        {
            return m_config.m_input.m_allowOverride;
        }

        bool ChannelSettingsBase::IsOutputOverrideAllowed() const
        {
            return m_config.m_output.m_allowOverride;
        }

        bool ChannelSettingsBase::IsSharedInputOutput() const
        {
            return m_config.m_shared;
        }

        const std::shared_ptr<SharedMemoryCache>& ChannelSettingsBase::GetMemoryCache() const
        {
            return m_cache;
        }

        void ChannelSettingsBase::Override(const char* commonInput, const char* commonOutput)
        {
            if (m_config.m_input.m_allowOverride && commonInput)
            {
                m_config.m_input.m_common = m_cache->Open(commonInput);
            }

            if (m_config.m_output.m_allowOverride && commonOutput)
            {
                m_config.m_output.m_common = m_cache->Open(commonOutput);
            }
        }

        auto ChannelSettingsBase::GetConfig() const -> const Config&
        {
            return m_config;
        }

    } // detail
} // IPC
