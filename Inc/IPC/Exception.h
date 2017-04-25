#pragma once

#include <stdexcept>


namespace IPC
{
    class Exception : public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };

} // IPC
