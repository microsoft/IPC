#pragma once

#include <msclr/gcroot.h>   // Needed to prevent vcclr.h to include the root gcroot.h without namespace.
#include <msclr/auto_gcroot.h>

#pragma managed(push, off)
#include <memory>
#pragma managed(pop)


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename Lambda>
        auto MakeManagedCallback(Lambda^ lambda)
        {
            return [lambda = std::shared_ptr<msclr::auto_gcroot<Lambda^>>{ new msclr::auto_gcroot<Lambda^>{ lambda } }](auto&&... args)
            {
                return lambda->get()->operator()(std::forward<decltype(args)>(args)...);
            };
        }

    } // detail

} // Managed
} // IPC
