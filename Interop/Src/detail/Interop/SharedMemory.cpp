#include "stdafx.h"
#include "IPC/Managed/detail/Interop/SharedMemory.h"
#include "IPC/SharedMemory.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        SharedMemory SharedMemory::Create(const char* name, std::size_t size)
        {
            return{ std::make_shared<IPC::SharedMemory>(create_only, name, size) };
        }

        SharedMemory SharedMemory::Open(const char* name)
        {
            return{ std::make_shared<IPC::SharedMemory>(open_only, name) };
        }

        SharedMemory::SharedMemory(const std::shared_ptr<IPC::SharedMemory>& memory)
            : shared_ptr{ memory }
        {}

        SharedMemory::SharedMemory(std::shared_ptr<IPC::SharedMemory>&& memory)
            : shared_ptr{ std::move(memory) }
        {}

        SharedMemory::~SharedMemory() = default;

        const char* SharedMemory::GetName() const
        {
            return get()->GetName().c_str();
        }

        std::size_t SharedMemory::GetFreeSize() const
        {
            return get()->GetFreeSize();
        }

    } // Interop
    } // detail

} // Managed
} // IPC
