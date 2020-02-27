#include "Schema.h"
#include <IPC/SharedMemory.h>
#include <memory>
#include <exception>
#include <sstream>
#include <iostream>


namespace Calc
{
    class Service
    {
    public:
        explicit Service(std::shared_ptr<IPC::SharedMemory> memory)
            : m_memory{ std::move(memory) }
        {
            std::cout << "Service constructed" << std::endl;
        }

        Service(const Service& other) = delete;
        Service& operator=(const Service& other) = delete;

        Service(Service&& other) = default;
        Service& operator=(Service&& other) = default;

        ~Service()
        {
            if (m_memory)
            {
                std::cout << "Service destructed" << std::endl;
            }
        }

        template <typename Callback>
        void operator()(const Request& request, Callback&& callback) const
        {
            Response response;
            std::ostringstream text;
            text << request.X << ' ';

            switch (request.Op)
            {
            case Operation::Add:
                response.Z = request.X + request.Y;
                text << '+';
                break;

            case Operation::Subtract:
                response.Z = request.X - request.Y;
                text << '-';
                break;

            case Operation::Multiply:
                response.Z = request.X * request.Y;
                text << '*';
                break;

            case Operation::Divide:
                response.Z = request.X / request.Y;
                text << '/';
                break;
            }

            text << ' ' << request.Y << " = ";
            response.Text.emplace(text.str().c_str(), m_memory->GetAllocator<char>());

            try
            {
                callback(std::move(response));
            }
            catch (const std::exception& e)
            {
                std::cout << "Failed to send response: " << e.what() << std::endl;
            }
        }

    private:
        std::shared_ptr<IPC::SharedMemory> m_memory;
    };
}
