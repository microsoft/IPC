#pragma once

#include <exception>
#include <iostream>


namespace IPC
{
namespace Policies
{
    class ErrorHandler
    {
    public:
        ErrorHandler(std::ostream& stream = std::cerr);

        void operator()(std::exception_ptr error) const;

    private:
        std::ostream& m_stream;
    };

} // Policies
} // IPC
