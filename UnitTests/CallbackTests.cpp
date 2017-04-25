#include "stdafx.h"
#include "IPC/detail/Callback.h"
#include <memory>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(CallbackTests)

static_assert(!std::is_copy_constructible<detail::Callback<void()>>::value, "Callback should not be copy constructible.");
static_assert(!std::is_copy_assignable<detail::Callback<void()>>::value, "Callback should not be copy assignable.");
static_assert(std::is_move_constructible<detail::Callback<void()>>::value, "Callback should be move constructible.");
static_assert(std::is_move_assignable<detail::Callback<void()>>::value, "Callback should be move assignable.");

BOOST_AUTO_TEST_CASE(EmptyCallbackTest)
{
    detail::Callback<void()> callback;
    BOOST_TEST(!callback);

    callback = [] {};
    BOOST_TEST(!!callback);

    auto other = std::move(callback);
    BOOST_TEST(!!other);
    BOOST_TEST(!callback);
}

BOOST_AUTO_TEST_CASE(MoveOnlyObjectCaptureTest)
{
    constexpr int value = 123;

    auto func = [x = std::make_unique<int>(value)]{ return x ? *x : throw std::exception{}; };

    detail::Callback<int()> callback{ std::move(func) };
    BOOST_TEST(callback() == value);
    BOOST_CHECK_THROW(func(), std::exception);
}

BOOST_AUTO_TEST_CASE(CopyableObjectCaptureTest)
{
    constexpr int value = 123;

    auto func = [x = std::make_shared<int>(value)]{ return x ? *x : throw std::exception{}; };

    {
        detail::Callback<int()> callback{ func };
        BOOST_TEST(callback() == value);
        BOOST_TEST(func() == value);
    }
    {
        detail::Callback<int()> callback{ std::move(func) };
        BOOST_TEST(callback() == value);
        BOOST_CHECK_THROW(func(), std::exception);
    }
}

BOOST_AUTO_TEST_SUITE_END()
