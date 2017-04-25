#pragma once

#include "Alias.h"
#include "IPC/SharedMemory.h"


namespace IPC
{
    namespace detail
    {
        template <typename T>
        class SharedObject : public SharedMemory::SharedPtr<T>
        {
            using SharedPtr = SharedMemory::SharedPtr<T>;

        public:
            template <typename... Args>
            SharedObject(create_only_t, SharedMemory& memory, const char* name, Args&&... args)
                : m_owner{ Create(memory, name, std::forward<Args>(args)...) }
            {
                SharedPtr::operator=(*m_owner);
            }

            SharedObject(open_only_t, SharedMemory& memory, const char* name)
                : SharedPtr{ Open(memory, name) }
            {}

        private:
            template <typename... Args>
            static auto Create(SharedMemory& memory, const char* name, Args&&... args)
            {
                auto obj = memory.MakeShared<T>(anonymous_instance, std::forward<Args>(args)...);
                return name ? memory.MakeShared<SharedPtr>(name, std::move(obj)) : memory.MakeShared<SharedPtr>(unique_instance, std::move(obj));
            }

            static auto Open(SharedMemory& memory, const char* name)
            {
                SharedPtr obj;
                memory.InvokeAtomic([&] { obj = name ? memory.Find<SharedPtr>(name) : memory.Find<SharedPtr>(unique_instance); });
                return obj;
            }

            SharedMemory::SharedPtr<SharedPtr> m_owner;
        };

    } // detail
} // IPC
