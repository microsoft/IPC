#pragma once

#include "Alias.h"
#include "KernelObject.h"


namespace IPC
{
    namespace detail
    {
        class KernelEvent : public KernelObject
        {
        public:
            KernelEvent(nullptr_t);

            KernelEvent(create_only_t, bool manualReset, bool initialState = false, const char* name = nullptr);

            KernelEvent(open_only_t, const char* name);

            bool Signal();

            bool Reset();
        };
        
    } // detail
} // IPC
