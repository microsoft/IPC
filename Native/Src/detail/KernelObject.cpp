#include "stdafx.h"
#include "IPC/detail/KernelObject.h"
#include "IPC/Exception.h"
#include <system_error>


namespace IPC
{
namespace detail
{
    namespace
    {
        HANDLE VerifyHandle(HANDLE handle, bool verifyValidHandle)
        {
            if (!handle || handle == INVALID_HANDLE_VALUE || verifyValidHandle)
            {
                auto error = ::GetLastError();
                if (error != ERROR_SUCCESS)
                {
                    try
                    {
                        throw std::system_error{ static_cast<int>(error), std::system_category() };
                    }
                    catch (const std::system_error&)
                    {
                        std::throw_with_nested(Exception{ "Invalid handle." });
                    }
                }
            }

            return handle;
        }
    }


    KernelObject::KernelObject(std::nullptr_t)
    {}

    KernelObject::KernelObject(void* handle, bool verifyValidHandle)
        : m_handle{ VerifyHandle(handle, verifyValidHandle), ::CloseHandle }
    {}

    void KernelObject::Wait() const
    {
        ::WaitForSingleObject(m_handle.get(), INFINITE);
    }

    bool KernelObject::Wait(std::chrono::milliseconds timeout) const
    {
        return ::WaitForSingleObject(m_handle.get(), static_cast<DWORD>(timeout.count())) == WAIT_OBJECT_0;
    }

    bool KernelObject::IsSignaled() const
    {
        return Wait(std::chrono::milliseconds::zero());
    }

    KernelObject::operator bool() const
    {
        return static_cast<bool>(m_handle);
    }

    KernelObject::operator void*() const
    {
        return m_handle.get();
    }
        
} // detail
} // IPC
