#include "SchemaInterop.h"
#include <IPC/Managed/detail/TransportImpl.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <>
        struct Convert<Calc::Request>
        {
            using type = Calc::Managed::Request^;

            static Calc::Request From(type% from)
            {
                return Calc::Request{ from->X, from->Y };
            }
        };

        template <>
        struct Convert<Calc::Managed::Request^>
        {
            using type = Calc::Request;

            static Calc::Managed::Request^ From(const type& from)
            {
                return gcnew Calc::Managed::Request{ from };
            }
        };


        template <>
        struct Convert<Calc::Response>
        {
            using type = Calc::Managed::Response^;

            static const Calc::Response& From(type% from)
            {
                return from->Impl;
            }
        };

        template <>
        struct Convert<Calc::Managed::Response^>
        {
            using type = Calc::Response;

            static Calc::Managed::Response^ From(const type& from)
            {
                return gcnew Calc::Managed::Response{ from };
            }
        };


        template Transport<Calc::Managed::Request^, Calc::Managed::Response^>;

    } // detail

} // Managed
} // IPC
