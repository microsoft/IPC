#pragma once

#include "Cast.h"

#pragma managed(push, off)
#include <guiddef.h>
#include <memory>
#pragma managed(pop)


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <>
        struct Convert<::GUID>
        {
            using type = System::Guid;

            static ::GUID From(type% from)
            {
                ::GUID guid;
                {
                    pin_ptr<unsigned char> bytes = &from.ToByteArray()[0];
                    std::memcpy(&guid, bytes, sizeof(guid));
                }
                return guid;
            }
        };


        template <>
        struct Convert<System::Guid>
        {
            using type = ::GUID;

            static System::Guid From(const type& from)
            {
                return System::Guid{ from.Data1, from.Data2, from.Data3, from.Data4[0], from.Data4[1], from.Data4[2], from.Data4[3], from.Data4[4], from.Data4[5], from.Data4[6], from.Data4[7] };
            }
        };

    } // detail

} // Managed
} // IPC
