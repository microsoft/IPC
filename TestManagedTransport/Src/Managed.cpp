#include "IPC/Managed/detail/TransportImpl.h"
#include "IPC/Managed/detail/Arithmetic.h"
#include "IPC/Containers/Managed/detail/Vector.h"


namespace IPC
{
namespace Containers
{
namespace Managed
{
    namespace detail
    {
        template Vector<int>;
        template Vector<IPC::Containers::Managed::IVector<int>^>;

    } // detail

} // Managed
} // Containers

namespace Managed
{
    namespace detail
    {
        template Transport<int, int>;
        template Transport<IPC::Containers::Managed::IVector<int>^, IPC::Containers::Managed::IVector<int>^>;

    } // detail

} // Managed
} // IPC
