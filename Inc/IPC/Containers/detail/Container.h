#pragma once

#include "IPC/SharedMemory.h"


namespace IPC
{
namespace Containers
{
    namespace detail
    {
        template <typename T>
        class Container : public SharedMemory::SharedPtr<T>
        {
            using Base = SharedMemory::SharedPtr<T>;

        public:
            explicit Container(std::shared_ptr<SharedMemory> memory)
                : Base{ memory->MakeShared<T>(anonymous_instance, memory->GetAllocator<typename T::value_type>()) }
            {}
        };

    } // detail
} // Containers
} // IPC
