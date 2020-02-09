#pragma once

#include "detail/Container.h"
#include <boost/interprocess/containers/vector.hpp>


namespace IPC
{
namespace Containers
{
    template <typename T>
    class Vector : public detail::Container<boost::interprocess::vector<T, SharedMemory::Allocator<T>>>
    {
        using Vector::Container::Container;
    };

} // Containers
} // IPC
