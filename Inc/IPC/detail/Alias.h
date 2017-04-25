#pragma once

#pragma warning(disable : 4503) // decorated name length exceeded, name was truncated

#pragma warning(push)
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/segment_manager_helper.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
        namespace ipc = boost::interprocess;

    } // detail

    using detail::ipc::create_only_t;
    using detail::ipc::create_only;

    using detail::ipc::open_only_t;
    using detail::ipc::open_only;

    using detail::ipc::anonymous_instance;
    using detail::ipc::unique_instance;

} // IPC
