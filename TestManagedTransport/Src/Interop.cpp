#include "IPC/Managed/detail/Interop/TransportImpl.h"
#include "IPC/Containers/Managed/detail/Interop/VectorImpl.h"


namespace IPC
{
namespace Containers
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template Vector<int>;
        template Vector<IPC::Containers::Vector<int>>;

    } // Interop
    } // detail

} // Managed
} // Containers

namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template Transport<int, int>;
        template Transport<IPC::Containers::Vector<int>, IPC::Containers::Vector<int>>;

    } // Interop
    } // detail

} // Managed
} // IPC
