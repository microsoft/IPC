#include "stdafx.h"
#include "IPC/Policies/InlineReceiverFactory.h"
#include <memory>

using namespace IPC;


BOOST_AUTO_TEST_SUITE(InlineReceiverFactoryTests)

BOOST_AUTO_TEST_CASE(InvocationTest)
{
    static std::shared_ptr<int> s_handler;

    struct Queue
    {
        std::size_t ConsumeAll(std::shared_ptr<int> handler)
        {
            s_handler = std::move(handler);
            return 10;
        }
    };

    Policies::InlineReceiverFactory factory;

    auto handler = std::make_shared<int>();

    Queue queue;
    auto receiver = factory(queue, handler);

    BOOST_TEST(receiver() == 10);
    BOOST_TEST(handler == s_handler);
}

BOOST_AUTO_TEST_SUITE_END()
