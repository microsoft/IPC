#include "stdafx.h"
#include "IPC/detail/RandomString.h"
#include <set>
#include <string>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(RandomStringTests)

BOOST_AUTO_TEST_CASE(UniquenessTest)
{
    std::set<std::string> set;
    constexpr std::size_t N = 100;

    for (std::size_t i = 0; i < N; ++i)
    {
        BOOST_TEST(set.insert(detail::GenerateRandomString()).second);
    }

    BOOST_TEST(set.size() == N);
}

BOOST_AUTO_TEST_SUITE_END()
