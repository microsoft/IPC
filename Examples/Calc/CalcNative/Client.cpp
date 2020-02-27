#include "Schema.h"
#include <IPC/Transport.h>
#include <boost/optional/optional_io.hpp>
#include <chrono>
#include <random>
#include <future>
#include <memory>
#include <iostream>
#include <signal.h>


namespace Calc
{
    void RunClient(const char* address)
    {
        static std::promise<void> s_exit;
        signal(SIGINT, [](int signal) { s_exit.set_value(); });
        std::cout << "Press Ctrl+C to exit." << std::endl;

        auto exit = s_exit.get_future();

        IPC::Transport<Request, Response> transport;

        std::cout << "Connecting to " << address << std::endl;

        auto clientAccessor = transport.ConnectClient(address, true);

        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> nums{ 1.f, 99.f };
        std::uniform_int_distribution<> op{ 0, 3 };

        for (std::shared_ptr<IPC::Transport<Request, Response>::Client> client;
                exit.wait_for(std::chrono::seconds{ 1 }) == std::future_status::timeout; )
        {
            if (!client)
            {
                try
                {
                    client = clientAccessor();
                }
                catch (const std::exception & e)
                {
                    std::cout << "Failed to access the client: " << e.what() << std::endl;
                    continue;
                }

                auto info = client->GetConnection().GetInputChannel().GetMemory()->GetName() + " -> "
                        + client->GetConnection().GetOutputChannel().GetMemory()->GetName();

                std::cout << "Connected: " << info << std::endl;

                client->GetConnection().RegisterCloseHandler(
                    [info]
                    {
                        std::cout << "Disconnected: " << info << std::endl;
                    },
                    true);
            }

            Request request;
            request.X = nums(rng);
            request.Y = nums(rng);
            request.Op = static_cast<Operation>(op(rng));

            boost::optional<Response> response;

            const auto begin = std::chrono::steady_clock::now();

            try
            {
                response = (*client)(std::move(request)).get();
            }
            catch (std::exception& e)
            {
                std::cout << "Failed to send request: " << e.what() << std::endl;
                client = {};
                continue;
            }

            const auto end = std::chrono::steady_clock::now();
            const auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);

            std::cout << response->Text << response->Z << " [" << latency.count() << "us]" << std::endl;
        }

        std::cout << "Exiting..." << std::endl;
    }
}
