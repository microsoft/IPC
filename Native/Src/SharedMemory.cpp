#include "stdafx.h"
#include "IPC/SharedMemory.h"
#include "IPC/Version.h"
#include <string>


namespace IPC
{
    namespace
    {
        const char* MakeVersionedName(const char* name)
        {
            thread_local std::string s_name{ IPC_VERSION_TOKEN "_" };
            s_name.resize(sizeof(IPC_VERSION_TOKEN "_") - 1);
            s_name += name;
            return s_name.c_str();
        }
    }


    SharedMemory::SharedMemory(create_only_t, const char* name, std::size_t size)
        : m_memory{ create_only, MakeVersionedName(name), size },
          m_name{ name }
    {}

    SharedMemory::SharedMemory(open_only_t, const char* name)
        : m_memory{ open_only, MakeVersionedName(name) },
          m_name{ name }
    {}

    bool SharedMemory::Contains(const void* ptr) const
    {
        return m_memory.belongs_to_segment(ptr);
    }

    std::size_t SharedMemory::GetFreeSize() const
    {
        return m_memory.get_free_memory();
    }

    std::size_t SharedMemory::GetMinSize()
    {
        return ManagedSharedMemory::segment_manager::get_min_size();
    }

    const std::string& SharedMemory::GetName() const
    {
        return m_name;
    }

} // IPC
