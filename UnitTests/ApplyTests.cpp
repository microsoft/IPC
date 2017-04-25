#include "stdafx.h"
#include "IPC/detail/Apply.h"
#include <tuple>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ApplyTests)

BOOST_AUTO_TEST_CASE(ApplyTupleTest)
{
    int vx = 123;
    float vf = 4.56f;
    const char* vs = "string";
    auto result = true;

    auto func = [&](int x, float f, const char* s)
    {
        BOOST_TEST(x == vx);
        BOOST_TEST(f == vf, boost::test_tools::tolerance(0.01));
        BOOST_TEST(s == vs);

        return result;
    };

    BOOST_TEST(detail::ApplyTuple(func, std::forward_as_tuple(vx, vf, vs)) == result);
}

BOOST_AUTO_TEST_SUITE_END()
