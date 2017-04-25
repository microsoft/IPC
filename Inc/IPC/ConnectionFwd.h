#pragma once


namespace IPC
{
    struct DefaultTraits;

    template <typename Input, typename Output, typename Traits = DefaultTraits>
    class Connection;

} // IPC
