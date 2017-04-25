#include "stdafx.h"
#include "IPC/Policies/ErrorHandler.h"
#include "IPC/detail/RandomString.h"
#include <sstream>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ErrorHandlerTests)

BOOST_AUTO_TEST_CASE(StandardExceptionTest)
{
    std::stringstream stream;

    Policies::ErrorHandler handler{ stream };

    BOOST_TEST(stream.str().empty());

    auto message = detail::GenerateRandomString();

    handler(std::make_exception_ptr(std::exception{ message.c_str() }));

    BOOST_TEST(stream.str().find(message) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(NonStandardExceptionTest)
{
    std::stringstream stream;

    Policies::ErrorHandler handler{ stream };

    BOOST_TEST(stream.str().empty());

    handler(std::make_exception_ptr(1.2));

    BOOST_TEST(!stream.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()
