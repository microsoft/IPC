#include "stdafx.h"
#include "IPC/detail/KernelObject.h"
#include "IPC/detail/KernelEvent.h"
#include "IPC/detail/KernelProcess.h"
#include "IPC/detail/RandomString.h"
#include <memory>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(KernelObjectTests)

static_assert(std::is_copy_constructible<detail::KernelObject>::value, "KernelObject should be copy constructible.");
static_assert(std::is_copy_assignable<detail::KernelObject>::value, "KernelObject should be copy assignable.");
static_assert(std::is_move_constructible<detail::KernelObject>::value, "KernelObject should be move constructible.");
static_assert(std::is_move_assignable<detail::KernelObject>::value, "KernelObject should be move assignable.");

BOOST_AUTO_TEST_CASE(KernelObjectTest)
{
    BOOST_TEST(!detail::KernelObject{ nullptr });

    ::SetLastError(ERROR_SUCCESS);
    BOOST_CHECK_NO_THROW(detail::KernelObject{ INVALID_HANDLE_VALUE });

    ::SetLastError(ERROR_INVALID_HANDLE);
    BOOST_CHECK_THROW((detail::KernelObject{ INVALID_HANDLE_VALUE, true }), std::exception);

    auto handle = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
    detail::KernelObject obj{ handle };

    BOOST_TEST(handle == static_cast<void*>(obj));

    BOOST_TEST(!obj.Wait(std::chrono::milliseconds{ 1 }));

    BOOST_TEST(::SetEvent(handle));
    BOOST_TEST((obj.Wait(), true));

    BOOST_TEST(::SetEvent(handle));
    BOOST_TEST(obj.IsSignaled());
}


static_assert(std::is_copy_constructible<detail::KernelEvent>::value, "KernelEvent should be copy constructible.");
static_assert(std::is_copy_assignable<detail::KernelEvent>::value, "KernelEvent should be copy assignable.");
static_assert(std::is_move_constructible<detail::KernelEvent>::value, "KernelEvent should be move constructible.");
static_assert(std::is_move_assignable<detail::KernelEvent>::value, "KernelEvent should be move assignable.");

BOOST_AUTO_TEST_CASE(KernelEventTest_AutoReset)
{
    detail::KernelEvent on{ create_only, false, true };
    BOOST_TEST(on.IsSignaled());
    BOOST_TEST(!on.IsSignaled());
    BOOST_TEST(on.Signal());
    BOOST_TEST(on.Reset());
    BOOST_TEST(!on.IsSignaled());

    detail::KernelEvent off{ create_only, false, false };
    BOOST_TEST(!off.IsSignaled());
    BOOST_TEST(off.Signal());
    BOOST_TEST(off.IsSignaled());
    BOOST_TEST(!off.IsSignaled());
}

BOOST_AUTO_TEST_CASE(KernelEventTest_ManualReset)
{
    detail::KernelEvent on{ create_only, true, true };
    BOOST_TEST(on.IsSignaled());
    BOOST_TEST(on.IsSignaled());
    BOOST_TEST(on.Reset());
    BOOST_TEST(!on.IsSignaled());

    detail::KernelEvent off{ create_only, true, false };
    BOOST_TEST(!off.IsSignaled());
    BOOST_TEST(off.Signal());
    BOOST_TEST(off.IsSignaled());
    BOOST_TEST(off.IsSignaled());
}

BOOST_AUTO_TEST_CASE(KernelEventTest_Named)
{
    auto name = detail::GenerateRandomString();

    BOOST_TEST(!detail::KernelEvent{ nullptr });

    detail::KernelEvent created{ create_only, false, false, name.c_str() };
    BOOST_TEST(!created.IsSignaled());

    BOOST_CHECK_THROW((detail::KernelEvent{ open_only, (name + "_invalid").c_str() }), std::exception);

    detail::KernelEvent opened{ open_only, name.c_str() };
    BOOST_TEST(!opened.IsSignaled());

    BOOST_TEST(created.Signal());
    BOOST_TEST(opened.IsSignaled());

    BOOST_TEST(!opened.IsSignaled());

    BOOST_TEST(opened.Signal());
    BOOST_TEST(created.IsSignaled());
}


static_assert(std::is_copy_constructible<detail::KernelProcess>::value, "KernelProcess should be copy constructible.");
static_assert(std::is_copy_assignable<detail::KernelProcess>::value, "KernelProcess should be copy assignable.");
static_assert(std::is_move_constructible<detail::KernelProcess>::value, "KernelProcess should be move constructible.");
static_assert(std::is_move_assignable<detail::KernelProcess>::value, "KernelProcess should be move assignable.");

BOOST_AUTO_TEST_CASE(KernelProcessTest)
{
    detail::KernelProcess process{ detail::KernelProcess::GetCurrentProcessId() };
    BOOST_TEST(static_cast<HANDLE>(process));
    BOOST_TEST(!process.IsSignaled());
}

BOOST_AUTO_TEST_SUITE_END()
