#include <iostream>


namespace Calc
{
    void RunServer(const char* address);

    void RunClient(const char* address);
}

int main(int argc, char* argv[])
{
    constexpr auto c_address = "ipc://calc";

    switch (argc)
    {
    case 2:
        if (std::strcmp(argv[1], "--server") == 0)
        {
            Calc::RunServer(c_address);
            break;
        }
        else if (std::strcmp(argv[1], "--client") == 0)
        {
            Calc::RunClient(c_address);
            break;
        }

    default:
        std::cout << "Pass --server or --client option." << std::endl;
        return 1;
    }

    return 0;
}
