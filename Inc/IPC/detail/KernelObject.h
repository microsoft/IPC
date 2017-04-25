#pragma once

#include <memory>
#include <chrono>
#include <type_traits>


namespace IPC
{
    namespace detail
    {
        class KernelObject
        {
        public:
            KernelObject(std::nullptr_t);

            explicit KernelObject(void* handle, bool verifyValidHandle = false);

            void Wait() const;

            bool Wait(std::chrono::milliseconds timeout) const;

            bool IsSignaled() const;

            explicit operator bool() const;

            explicit operator void*() const;

        private:
            std::shared_ptr<void> m_handle;
        };
        
    } // detail
} // IPC
