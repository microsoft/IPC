#pragma once

#include <type_traits>

#pragma warning(push)
#include <boost/version.hpp>
#include <boost/config.hpp>
#pragma warning(pop)

#if _MSC_VER < 1800
#error Unsupported compiler version
#elif _MSC_VER < 1900
#define IPC_MSVC_MAJOR_VERSION 18
#elif _MSC_VER < 2000
#define IPC_MSVC_MAJOR_VERSION 19
#elif _MSC_VER < 2100
#define IPC_MSVC_MAJOR_VERSION 20
#else
#error Unknown compiler version
#endif

#define IPC_COMPILER_VERSION    "MSVC-" BOOST_STRINGIZE(IPC_MSVC_MAJOR_VERSION) "00"

#define IPC_BOOST_VERSION       "BOOST-" BOOST_STRINGIZE(BOOST_VERSION)

#ifdef NDEBUG
#define IPC_LIB_NAME            "IPC"
#else
#define IPC_LIB_NAME            "IPCD"
#endif

#define IPC_VERSION             1000    // XYYY, X=major, YYY=minor

#define IPC_LIB_VERSION         IPC_LIB_NAME "-" BOOST_STRINGIZE(IPC_VERSION)

#define IPC_VERSION_TOKEN       IPC_COMPILER_VERSION "_" IPC_BOOST_VERSION "_" IPC_LIB_VERSION


namespace IPC
{
    template <typename T>
    struct Version : std::integral_constant<std::size_t, 0> // Specialize and change (bump) version when breaking ABI of T.
    {};

} // IPC
