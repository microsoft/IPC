#pragma once

#include <cstdlib>


namespace IPC
{
    namespace detail
    {
    namespace LockFree
    {
        template <typename T, typename Allocator, std::size_t BucketSize = 64>
        class Queue;

    } // LockFree
    } // detail
} // IPC
