#include "stdafx.h"
#include "IPC/SharedMemoryCache.h"
#include "IPC/SharedMemory.h"
#include "IPC/detail/RandomString.h"

using namespace IPC;


BOOST_AUTO_TEST_SUITE(SharedMemoryCacheTests)

BOOST_AUTO_TEST_CASE(ConstructionTest)
{
    SharedMemoryCache cache;

    auto name = detail::GenerateRandomString();

    auto created = cache.Create(name.c_str(), 1024);
    BOOST_TEST(!!created);
    BOOST_TEST(created->GetName() == name);

    auto opened = cache.Open(name.c_str());
    BOOST_TEST(opened == created);

    BOOST_CHECK_THROW(cache.Open(detail::GenerateRandomString().c_str()), std::exception);
}

BOOST_AUTO_TEST_CASE(OwnershipTest)
{
    SharedMemoryCache cache;

    auto name = detail::GenerateRandomString();
    {
        auto created = cache.Create(name.c_str(), 1024);
        auto opened = cache.Open(name.c_str());
        BOOST_TEST(created == opened);
    }

    BOOST_CHECK_THROW(cache.Open(name.c_str()), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
