#pragma once

#include "IPC/ChannelSettings.h"
#include "IPC/InputChannel.h"
#include "IPC/OutputChannel.h"


namespace IPC
{
    namespace detail
    {
        template <typename Traits>
        class ChannelFactory;

        template <>
        class ChannelFactory<void>
        {
        public:
            class InstanceBase
            {
            protected:
                std::shared_ptr<SharedMemory> GetMemory(create_only_t, bool input, const char* name, const ChannelSettingsBase& settings);

                std::shared_ptr<SharedMemory> GetMemory(open_only_t, bool input, const char* name, const ChannelSettingsBase& settings);

            private:
                std::shared_ptr<SharedMemory> GetMemory(bool open, bool input, const char* name, const ChannelSettingsBase& settings);


                std::weak_ptr<SharedMemory> m_current;
            };
        };

        template <typename Traits>
        class ChannelFactory : public ChannelSettings<Traits>
        {
        public:
            class Instance : public ChannelFactory<void>::InstanceBase, public ChannelSettings<Traits>
            {
            public:
                explicit Instance(ChannelSettings<Traits> settings)
                    : Instance::ChannelSettings{ std::move(settings) }
                {}

                template <typename T>
                auto CreateInput(const char* name)
                {
                    return MakeInput<T>(create_only, name);
                }

                template <typename T>
                auto OpenInput(const char* name)
                {
                    return MakeInput<T>(open_only, name);
                }

                template <typename T>
                auto CreateOutput(const char* name)
                {
                    return MakeOutput<T>(create_only, name);
                }

                template <typename T>
                auto OpenOutput(const char* name)
                {
                    return MakeOutput<T>(open_only, name);
                }

            private:
                template <typename T, typename OpenOrCreate>
                auto MakeInput(OpenOrCreate openOrCreate, const char* name)
                {
                    return InputChannel<T, Traits>{
                        openOrCreate, name, GetMemory(openOrCreate, true, name, *this), this->GetWaitHandleFactory(), this->GetReceiverFactory() };
                }

                template <typename T, typename OpenOrCreate>
                auto MakeOutput(OpenOrCreate openOrCreate, const char* name)
                {
                    return OutputChannel<T, Traits>{ openOrCreate, name, GetMemory(openOrCreate, false, name, *this) };
                }
            };


            explicit ChannelFactory(ChannelSettings<Traits> settings)
                : ChannelFactory::ChannelSettings{ std::move(settings) }
            {}

            auto MakeInstance() const
            {
                return Instance{ *this };
            }

            auto Override(const char* commonInput, const char* commonOutput) const
            {
                auto newSettings = static_cast<const ChannelSettings<Traits>&>(*this);

                newSettings.Override(commonInput, commonOutput);

                return ChannelFactory{ std::move(newSettings) };
            }
        };

    } // detail

} // IPC
