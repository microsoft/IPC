#pragma once

#include <stdexcept>

#pragma make_public(std::exception)


namespace IPC
{
namespace Managed
{
    [System::SerializableAttribute]
    public ref class Exception : public System::Exception
    {
    public:
        Exception(const char* message);

        Exception(const std::exception& e);

        Exception(System::String^ message);
    };

} // Managed
} // IPC
