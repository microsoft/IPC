#pragma once

#include "KernelObject.h"


namespace IPC
{
    namespace detail
    {
        class KernelProcess : public KernelObject
        {
        public:
            explicit KernelProcess(std::uint32_t pid);

            static std::uint32_t GetCurrentProcessId();
        };
        
    } // detail
} // IPC
