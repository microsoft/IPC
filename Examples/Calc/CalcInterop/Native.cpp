#include "Schema.h"
#include <IPC/Managed/detail/Interop/TransportImpl.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template Transport<Calc::Request, Calc::Response>;

    } // Interop
    } // detail

} // Managed
} // IPC
