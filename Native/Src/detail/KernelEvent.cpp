#include "stdafx.h"
#include "IPC/detail/KernelEvent.h"


namespace IPC
{
namespace detail
{
    KernelEvent::KernelEvent(nullptr_t)
        : KernelObject{ nullptr }
    {}

    KernelEvent::KernelEvent(create_only_t, bool manualReset, bool initialState, const char* name)
        : KernelObject{ ::CreateEventA(nullptr, manualReset, initialState, name), true }
    {}

    KernelEvent::KernelEvent(open_only_t, const char* name)
        : KernelObject{ ::OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, name) }
    {}

    bool KernelEvent::Signal()
    {
        return ::SetEvent(static_cast<void*>(*this)) != FALSE;
    }

    bool KernelEvent::Reset()
    {
        return ::ResetEvent(static_cast<void*>(*this)) != FALSE;
    }
        
} // detail
} // IPC
