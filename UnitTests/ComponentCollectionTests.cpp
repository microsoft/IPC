#include "stdafx.h"
#pragma warning(push)
#pragma warning(disable : 4702) // Unreachable code.
#include "IPC/ComponentCollection.h"
#include "IPC/detail/Callback.h"
#include <memory>
#include <type_traits>
#include <future>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ComponentCollectionTests)

static_assert(!std::is_copy_constructible<ComponentCollection<std::list<int>>>::value, "ComponentCollection should not be copy constructible.");
static_assert(!std::is_copy_assignable<ComponentCollection<std::list<int>>>::value, "ComponentCollection should not be copy assignable.");
static_assert(std::is_move_constructible<ComponentCollection<std::list<int>>>::value, "ComponentCollection should be move constructible.");
static_assert(std::is_move_assignable<ComponentCollection<std::list<int>>>::value, "ComponentCollection should be move assignable.");

BOOST_AUTO_TEST_CASE(InsertionTest)
{
    ComponentCollection<std::list<int>> collection;

    detail::Callback<void()> eraser;
    collection.Accept([&](auto e) { eraser = std::move(e); return 100; });
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->size() == 1);
        BOOST_TEST(components->front() == 100);
    }
}

BOOST_AUTO_TEST_CASE(RemovalTest)
{
    ComponentCollection<std::list<int>> collection;

    detail::Callback<void()> eraser1, eraser2;

    collection.Accept([&](auto e) { eraser1 = std::move(e); return 100; });
    collection.Accept([&](auto e) { eraser2 = std::move(e); return 200; });
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->size() == 2);
        BOOST_TEST(components->front() == 100);
        BOOST_TEST(components->back() == 200);
    }
    eraser1();
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->size() == 1);
        BOOST_TEST(components->front() == 200);
    }
    eraser2();
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->empty());
    }
}

BOOST_AUTO_TEST_CASE(ErasorDestructionTest)
{
    ComponentCollection<std::list<int>> collection;
    {
        detail::Callback<void()> eraser;

        collection.Accept([&](auto e) { eraser = std::move(e); return 100; });
        {
            auto components = collection.GetComponents();
            BOOST_TEST(components->size() == 1);
            BOOST_TEST(components->front() == 100);
        }
    }
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->empty());
    }
}

BOOST_AUTO_TEST_CASE(DestructionTest)
{
    auto context = std::make_shared<int>();
    {
        using Component = std::pair<std::shared_ptr<int>, detail::Callback<void()>>;

        ComponentCollection<std::list<Component>> collection;

        collection.Accept([&](auto eraser) { return std::make_pair(context, std::move(eraser)); });
        collection.Accept([&](auto eraser) { return std::make_pair(context, std::move(eraser)); });
        {
            auto components = collection.GetComponents();
            BOOST_TEST(components->size() == 2);
            BOOST_TEST(components->front().first == context);
            BOOST_TEST(components->back().first == context);
        }
        BOOST_TEST(context.use_count() == 3);
    }
    BOOST_TEST(context.unique());
}

BOOST_AUTO_TEST_CASE(SelfDestructionTest)
{
    ComponentCollection<std::list<int>> collection;

    collection.Accept([&](auto eraser) { eraser(); return 0; });
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->empty());
    }
    collection.Accept([&](auto) { return 0; });
    {
        auto components = collection.GetComponents();
        BOOST_TEST(components->empty());
    }
}

BOOST_AUTO_TEST_CASE(ExceptionSafetyTest)
{
    ComponentCollection<std::list<int>> collection;

    BOOST_CHECK_THROW(collection.Accept([](auto) -> int { throw std::exception{}; }), std::exception);
}

BOOST_AUTO_TEST_CASE(ComponentsAccessThreadSafetyTest)
{
    ComponentCollection<std::list<int>> collection;
    {
        auto components = collection.GetComponents();
        auto holder = std::make_unique<decltype(components)>(std::move(components));

        detail::Callback<void()> eraser;
        auto result = std::async(std::launch::async,
            [&]
            {
                collection.Accept([&](auto e) { eraser = std::move(e); return 0; });
                return true;
            });

        BOOST_TEST((result.wait_for(std::chrono::milliseconds{ 1 }) == std::future_status::timeout));
        holder.reset();
        BOOST_TEST(result.get());
    }
}

BOOST_AUTO_TEST_SUITE_END()

#pragma warning(pop)
