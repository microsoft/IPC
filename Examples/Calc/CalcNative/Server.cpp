#include "Schema.h"
#include "Service.h"
#include <IPC/Transport.h>
#include <future>
#include <iostream>
#include <signal.h>


namespace Calc
{
    void RunServer(const char* address)
    {
        IPC::Transport<Request, Response> transport;

        std::cout << "Hosting server at " << address << std::endl;

        auto serversAccessor = transport.AcceptServers(
            address,
            [](auto& connection)
            {
                const auto info = connection.GetInputChannel().GetMemory()->GetName() + " -> "
                                + connection.GetOutputChannel().GetMemory()->GetName();

                std::cout << "Connected: " << info << std::endl;

                connection.RegisterCloseHandler([info] { std::cout << "Disconnected: " << info << std::endl; }, true);

                return Service{ connection.GetOutputChannel().GetMemory() };
            });

        static std::promise<void> s_exit;
        signal(SIGINT, [](int signal) { s_exit.set_value(); });
        std::cout << "Press Ctrl+C to exit." << std::endl;

        s_exit.get_future().wait();
        std::cout << "Exiting..." << std::endl;
    }
}
