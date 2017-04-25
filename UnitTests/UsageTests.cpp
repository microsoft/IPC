#include "stdafx.h"
#include "IPC/Accept.h"
#include "IPC/Acceptor.h"
#include "IPC/Connect.h"
#include "IPC/Connector.h"
#include "IPC/detail/RandomString.h"
#include <cmath>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <future>

#pragma warning(push)
#include <boost/interprocess/containers/string.hpp>
#include <boost/optional.hpp>
#pragma warning(pop)


BOOST_AUTO_TEST_SUITE(UsageTests)

BOOST_AUTO_TEST_CASE(AcceptorConnectorTest)
{
    auto serverHandler = [](int x, auto&& callback)
    {
        auto y = std::sqrt(x);

        try
        {
            callback(y);
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto closeHandler = []
    {
        std::clog << "Connection is closed." << std::endl;
    };

    auto name = IPC::detail::GenerateRandomString();

    std::mutex lock;
    std::condition_variable serverInserted;
    std::vector<std::unique_ptr<IPC::Server<int, double>>> servers;

    IPC::ServerAcceptor<int, double> acceptor{
        name.c_str(),
        [&](auto futureConnection)
        {
            try
            {
                std::lock_guard<std::mutex> guard{ lock };
                servers.push_back(std::make_unique<IPC::Server<int, double>>(futureConnection.get(), serverHandler, closeHandler));
                serverInserted.notify_one();
            }
            catch (const std::exception& e)
            {
                std::clog << "Failed to accept a server: " << e.what() << std::endl;
            }
        } };

    std::unique_ptr<IPC::Client<int, double>> client;

    IPC::ClientConnector<int, double> connector;

    try
    {
        client = std::make_unique<IPC::Client<int, double>>(connector.Connect(name.c_str()).get(), closeHandler);
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to connect a client: " << e.what() << std::endl;
    }

    {
        std::unique_lock<std::mutex> guard{ lock };
        serverInserted.wait(guard, [&] { return !servers.empty(); });
    }

    BOOST_TEST(servers.size() == 1);

    int x = 111;
    double y;

    try
    {
        y = (*client)(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y == std::sqrt(x), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(ReverseConnectionAcceptorConnectorTest)
{
    auto serverHandler = [](int x, auto&& callback)
    {
        auto y = std::sqrt(x);

        try
        {
            callback(y);
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto closeHandler = []
    {
        std::clog << "Connection is closed." << std::endl;
    };

    auto name = IPC::detail::GenerateRandomString();

    std::mutex lock;
    std::condition_variable clientInserted;
    std::vector<std::unique_ptr<IPC::Client<int, double>>> clients;

    IPC::ClientAcceptor<int, double> acceptor{
        name.c_str(),
        [&](auto futureConnection)
        {
            try
            {
                std::lock_guard<std::mutex> guard{ lock };
                clients.push_back(std::make_unique<IPC::Client<int, double>>(futureConnection.get(), closeHandler));
                clientInserted.notify_one();
            }
            catch (const std::exception& e)
            {
                std::clog << "Failed to accept a client: " << e.what() << std::endl;
            }
        } };

    std::unique_ptr<IPC::Server<int, double>> server;

    IPC::ServerConnector<int, double> connector;

    try
    {
        server = std::make_unique<IPC::Server<int, double>>(connector.Connect(name.c_str()).get(), serverHandler, closeHandler);
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to connect a server: " << e.what() << std::endl;
    }

    {
        std::unique_lock<std::mutex> guard{ lock };
        clientInserted.wait(guard, [&] { return !clients.empty(); });
    }

    BOOST_TEST(clients.size() == 1);

    int x = 222;
    double y;

    try
    {
        y = (*clients.front())(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y == std::sqrt(x), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(AcceptConnectTest)
{
    auto serverHandler = [](int x, auto&& callback)
    {
        auto y = std::sqrt(x);

        try
        {
            callback(y);
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto name = IPC::detail::GenerateRandomString();

    auto serversAccessor = IPC::AcceptServers<int, double>(name.c_str(), [&](auto&&...) { return serverHandler; });

    auto clientAccessor = IPC::ConnectClient(name.c_str(), std::make_shared<IPC::ClientConnector<int, double>>(), false);

    std::shared_ptr<IPC::Client<int, double>> client;

    try
    {
        client = clientAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Client is not available: " << e.what() << std::endl;
    }

    assert(client);

    BOOST_TEST(serversAccessor()->size() == 1);

    int x = 333;
    double y;

    try
    {
        y = (*client)(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y == std::sqrt(x), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(ReverseConnectionAcceptConnectTest)
{
    auto serverHandler = [](int x, auto&& callback)
    {
        auto y = std::sqrt(x);

        try
        {
            callback(y);
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto name = IPC::detail::GenerateRandomString();

    auto clientsAccessor = IPC::AcceptClients<int, double>(name.c_str());

    auto serverAccessor = IPC::ConnectServer(
        name.c_str(),
        std::make_shared<IPC::ServerConnector<int, double>>(),
        [&](auto&&...) { return serverHandler; },
        false);

    std::shared_ptr<IPC::Server<int, double>> server;

    try
    {
        server = serverAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Server is not available: " << e.what() << std::endl;
    }

    assert(server);

    BOOST_TEST(clientsAccessor()->size() == 1);

    int x = 444;
    double y;

    try
    {
        y = (*clientsAccessor()->front())(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y == std::sqrt(x), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(DynamicDataTest)
{
    using String = boost::interprocess::basic_string<char, std::char_traits<char>, IPC::SharedMemory::Allocator<char>>;
    using OptionalString = boost::optional<String>; // Using optional string due to bugs in std::future<T>.

    auto serverHandler = [](const IPC::SharedMemory::Allocator<char>& allocator, OptionalString&& str, auto&& callback)
    {
        String result{ str ? str->c_str() : "", allocator };
        std::transform(result.begin(), result.end(), result.begin(), [](char c) { return char(std::toupper(c)); });

        try
        {
            callback(std::move(result));
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto name = IPC::detail::GenerateRandomString();

    auto serversAccessor = IPC::AcceptServers<OptionalString, OptionalString>(
        name.c_str(),
        [&](const auto& connection)
        {
            auto allocator = connection.GetOutputChannel().GetMemory()->GetAllocator<char>();
            return [&, allocator = std::move(allocator)](OptionalString&& str, auto&& callback)
            {
                return serverHandler(allocator, std::move(str), std::forward<decltype(callback)>(callback));
            };
        });

    auto clientAccessor = IPC::ConnectClient(name.c_str(), std::make_shared<IPC::ClientConnector<OptionalString, OptionalString>>(), false);

    std::shared_ptr<IPC::Client<OptionalString, OptionalString>> client;

    try
    {
        client = clientAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Client is not available: " << e.what() << std::endl;
    }

    assert(client);

    BOOST_TEST(serversAccessor()->size() == 1);

    std::string str = "ipc";
    OptionalString result;

    try
    {
        auto allocator = client->GetConnection().GetOutputChannel().GetMemory()->GetAllocator<char>();

        result = (*client)(String{ str.c_str(), allocator }).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(!!result);
    BOOST_TEST(*result == "IPC");
}

BOOST_AUTO_TEST_CASE(StressTest)
{
    // TODO:
}

BOOST_AUTO_TEST_SUITE_END()
