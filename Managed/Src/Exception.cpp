#include "Exception.h"


namespace IPC
{
namespace Managed
{
    Exception::Exception(const char* message)
        : Exception{ gcnew System::String{ message } }
    {}

    Exception::Exception(const std::exception& e)
        : Exception{ e.what() }
    {}

    Exception::Exception(System::String^ message)
        : System::Exception{ message }
    {}

} // Managed
} // IPC
