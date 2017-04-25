#include "stdafx.h"
#include "IPC/detail/RandomString.h"

#pragma warning(push)
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
        std::string GenerateRandomString()
        {
            thread_local boost::uuids::random_generator s_generator{};
            return to_string(s_generator());
        }

    } // detail
} // IPC
