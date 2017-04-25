#include "stdafx.h"
#include "IPC/Policies/ThreadPool.h"
#include <memory>
#include <future>
#include <type_traits>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(ThreadPoolTests)

static_assert(std::is_copy_constructible<Policies::ThreadPool>::value, "ThreadPool should be copy constructible.");
static_assert(std::is_copy_assignable<Policies::ThreadPool>::value, "ThreadPool should be copy assignable.");
static_assert(std::is_move_constructible<Policies::ThreadPool>::value, "ThreadPool should be move constructible.");
static_assert(std::is_move_assignable<Policies::ThreadPool>::value, "ThreadPool should be move assignable.");

BOOST_AUTO_TEST_CASE(ConstructionTest)
{
    BOOST_CHECK_NO_THROW(Policies::ThreadPool{});

    BOOST_CHECK_NO_THROW((Policies::ThreadPool{ 0, 10 }));

    BOOST_CHECK_THROW((Policies::ThreadPool{ 3, 1 }), std::exception);
}

BOOST_AUTO_TEST_CASE(AsyncWorkTest)
{
    Policies::ThreadPool pool;
    std::promise<bool> result;

    std::shared_ptr<TP_WORK> work{
        ::CreateThreadpoolWork(
            static_cast<PTP_WORK_CALLBACK>([](PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK)
            {
                static_cast<std::promise<bool>*>(context)->set_value(true);
            }),
            &result,
            static_cast<PTP_CALLBACK_ENVIRON>(pool.GetEnvironment())),
        ::CloseThreadpoolWork };

    BOOST_TEST(work);
    ::SubmitThreadpoolWork(work.get());
    BOOST_TEST(result.get_future().get());
}

BOOST_AUTO_TEST_SUITE_END()
