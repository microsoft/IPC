#include "stdafx.h"
#include "IPC/detail/KernelProcess.h"


namespace IPC
{
namespace detail
{
    KernelProcess::KernelProcess(std::uint32_t pid)
        : KernelObject{ ::OpenProcess(SYNCHRONIZE, false, pid) }
    {}

    std::uint32_t KernelProcess::GetCurrentProcessId()
    {
        return ::GetCurrentProcessId();
    }
        
} // detail
} // IPC
