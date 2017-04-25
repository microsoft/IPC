#include "Exception.h"
#include "SharedMemory.h"
#include "IPC/Managed/detail/Throw.h"

#include <msclr/marshal.h>


namespace IPC
{
namespace Managed
{
    SharedMemory^ SharedMemory::Create(System::String^ name, System::Int32 size)
    {
        try
        {
            return gcnew SharedMemory{
                detail::Interop::SharedMemory::Create(msclr::interop::marshal_context().marshal_as<const char*>(name), size) };
        }
        catch (const std::exception& /*e*/)
        {
            detail::ThrowManagedException(std::current_exception());
        }
    }

    SharedMemory^ SharedMemory::Open(System::String^ name)
    {
        try
        {
            return gcnew SharedMemory{
                detail::Interop::SharedMemory::Open(msclr::interop::marshal_context().marshal_as<const char*>(name)) };
        }
        catch (const std::exception& /*e*/)
        {
            detail::ThrowManagedException(std::current_exception());
        }
    }

    SharedMemory::SharedMemory(const detail::Interop::SharedMemory& memory)
        : m_memory{ memory }
    {}

    const detail::Interop::SharedMemory& SharedMemory::Impl::get()
    {
        return *m_memory;
    }

    System::String^ SharedMemory::Name::get()
    {
        return gcnew System::String{ m_memory->GetName() };
    }

    System::Int32 SharedMemory::FreeSize::get()
    {
        return static_cast<System::Int32>(m_memory->GetFreeSize());
    }

} // Managed
} // IPC
