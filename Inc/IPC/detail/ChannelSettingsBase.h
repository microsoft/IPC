#pragma once

#include <IPC/SharedMemoryCache.h>
#include <IPC/SharedMemory.h>
#include <memory>


namespace IPC
{
    namespace detail
    {
        class ChannelSettingsBase
        {
        public:
            void SetInput(std::size_t memorySize, bool allowOverride);

            void SetOutput(std::size_t memorySize, bool allowOverride);

            void SetInput(std::shared_ptr<SharedMemory> memory, bool allowOverride);

            void SetOutput(std::shared_ptr<SharedMemory> memory, bool allowOverride);

            void SetInputOutput(std::size_t memorySize, bool allowOverride);

            void SetInputOutput(std::shared_ptr<SharedMemory> memory, bool allowOverride);

            const std::shared_ptr<SharedMemory>& GetCommonInput() const;

            const std::shared_ptr<SharedMemory>& GetCommonOutput() const;

            bool IsInputOverrideAllowed() const;

            bool IsOutputOverrideAllowed() const;

            bool IsSharedInputOutput() const;

            const std::shared_ptr<SharedMemoryCache>& GetMemoryCache() const;

        protected:
            explicit ChannelSettingsBase(std::shared_ptr<SharedMemoryCache> cache);

        private:
            template <typename Traits>
            friend class ChannelFactory;

            struct ChannelConfig
            {
                std::size_t m_size{ 1024 * 1024 };
                std::shared_ptr<SharedMemory> m_common;
                bool m_allowOverride{ true };
            };

            struct Config
            {
                ChannelConfig m_input;
                ChannelConfig m_output;
                bool m_shared{ false };
            };


            void Override(const char* commonInput, const char* commonOutput);

            const Config& GetConfig() const;


            Config m_config;
            std::shared_ptr<SharedMemoryCache> m_cache;
        };

    } // detail
} // IPC
