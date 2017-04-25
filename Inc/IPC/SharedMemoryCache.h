#pragma once

#include <memory>


namespace IPC
{
    class SharedMemory;


    class SharedMemoryCache
    {
    public:
        SharedMemoryCache();

        ~SharedMemoryCache();

        std::shared_ptr<SharedMemory> Create(const char* name, std::size_t size);

        std::shared_ptr<SharedMemory> Open(const char* name);

    private:
        class Impl;

        std::unique_ptr<Impl> m_impl;
    };

} // IPC
