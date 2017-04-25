#pragma once
#pragma managed(push, off)

#include <memory>


namespace IPC
{

class SharedMemory;

namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        class SharedMemory : public std::shared_ptr<IPC::SharedMemory>
        {
        public:
            static SharedMemory Create(const char* name, std::size_t size);

            static SharedMemory Open(const char* name);

            SharedMemory(const std::shared_ptr<IPC::SharedMemory>& memory);

            SharedMemory(std::shared_ptr<IPC::SharedMemory>&& memory);

            ~SharedMemory();

            const char* GetName() const;

            std::size_t GetFreeSize() const;
        };

    } // Interop
    } // detail

} // Managed
} // IPC

#pragma managed(pop)
