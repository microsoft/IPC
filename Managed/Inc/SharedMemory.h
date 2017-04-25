#pragma once

#include "IPC/Managed/detail/NativeObject.h"
#include "IPC/Managed/detail/Interop/SharedMemory.h"

#pragma make_public(IPC::Managed::detail::Interop::SharedMemory)


namespace IPC
{
namespace Managed
{
    public ref class SharedMemory
    {
    public:
        static SharedMemory^ Create(System::String^ name, System::Int32 size);

        static SharedMemory^ Open(System::String^ name);

        explicit SharedMemory(const detail::Interop::SharedMemory& memory);

        property const detail::Interop::SharedMemory& Impl
        {
            const detail::Interop::SharedMemory& get();
        }

        property System::String^ Name
        {
            System::String^ get();
        }

        property System::Int32 FreeSize
        {
            System::Int32 get();
        }

    private:
        detail::NativeObject<detail::Interop::SharedMemory> m_memory;
    };

} // Managed
} // IPC
