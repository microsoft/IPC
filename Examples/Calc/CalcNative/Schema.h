#pragma once

#include <IPC/SharedMemory.h>
#include <boost/optional.hpp>
#include <boost/interprocess/containers/string.hpp>


namespace Calc
{
    enum class Operation
    {
        Add, Subtract, Multiply, Divide
    };

    struct Request
    {
        float X{};
        float Y{};
        Operation Op{ Operation::Add };
    };

    struct Response
    {
        float Z{};
        boost::optional<boost::interprocess::basic_string<char, std::char_traits<char>, IPC::SharedMemory::Allocator<char>>> Text;
    };
}
