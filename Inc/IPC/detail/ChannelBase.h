#pragma once

#include "IPC/SharedMemory.h"
#include "IPC/Version.h"
#include "Alias.h"
#include "RandomString.h"
#include "KernelEvent.h"
#include "SharedObject.h"
#include <string>
#include <typeinfo>
#include <memory>
#include <atomic>

#pragma warning(push)
#include <boost/interprocess/containers/string.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
        namespace
        {
            template <typename T>
            const char* MakeVersionedName(const char* name)
            {
                static const std::string s_versionedTypeName = std::string{ typeid(T).name() } + "_V" + std::to_string(Version<T>::value) + "_";
                thread_local std::string s_name = s_versionedTypeName;

                s_name.resize(s_versionedTypeName.size());

                if (name)
                {
                    s_name += name;
                }

                return s_name.c_str();
            }
        }


        template <typename T, template <typename U, typename Allocator> typename QueueT>
        class ChannelBase
        {
        public:
            ChannelBase(create_only_t, const char* name, std::shared_ptr<SharedMemory> memory)
                : m_memory{ std::move(memory) },
                  m_queue{ create_only, *m_memory, MakeVersionedName<DataQueue>(name), m_memory->GetAllocator<void>() },
                  m_notEmptyEvent{ create_only, false, false, m_queue->m_notEmptyEventName.c_str() }
            {}

            ChannelBase(open_only_t, const char* name, std::shared_ptr<SharedMemory> memory)
                : m_memory{ std::move(memory) },
                  m_queue{ open_only, *m_memory, MakeVersionedName<DataQueue>(name) },
                  m_notEmptyEvent{ open_only, m_queue->m_notEmptyEventName.c_str() }
            {}

            ChannelBase(const ChannelBase& other) = delete;
            ChannelBase& operator=(const ChannelBase& other) = delete;

            ChannelBase(ChannelBase&& other) = default;
            ChannelBase& operator=(ChannelBase&& other) = default;

            bool IsEmpty() const
            {
                return m_queue->IsEmpty();
            }

            const std::shared_ptr<SharedMemory>& GetMemory() const
            {
                return m_memory;
            }

        protected:
            using Queue = QueueT<T, SharedMemory::Allocator<T>>;

            Queue& GetQueue()
            {
                return *m_queue;
            }

            auto& GetNotEmptyEvent()
            {
                return m_notEmptyEvent;
            }

            auto& GetCounter()
            {
                return m_queue->m_count;
            }

        private:
            using String = ipc::basic_string<char, std::char_traits<char>, SharedMemory::Allocator<char>>;

            struct DataQueue : Queue
            {
                explicit DataQueue(const SharedMemory::Allocator<char>& allocator)
                    : ChannelBase::Queue{ allocator }, // TODO: Use "Queue" when VC14 bugs are fixed.
                      m_notEmptyEventName{ GenerateRandomString().c_str(), allocator }
                {}

                const String m_notEmptyEventName;
                std::atomic_size_t m_count{ 0 };
            };

            std::shared_ptr<SharedMemory> m_memory;
            SharedObject<DataQueue> m_queue;
            KernelEvent m_notEmptyEvent;
        };

    } // detail
} // IPC
