#include "stdafx.h"
#include "IPC/SharedMemoryCache.h"
#include "IPC/SharedMemory.h"
#include <unordered_map>
#include <string>
#include <shared_mutex>


namespace IPC
{
    class SharedMemoryCache::Impl
    {
    public:
        std::shared_ptr<SharedMemory> Create(const char* name, std::size_t size)
        {
            auto memory = std::make_shared<SharedMemory>(create_only, name, size);

            {
                std::lock_guard<decltype(m_lock)> guard{ m_lock };
                m_cache[name] = memory;
            }

            return memory;
        }

        std::shared_ptr<SharedMemory> Open(const char* name)
        {
            if (auto memory = TryOpen(name))
            {
                return memory;
            }

            std::lock_guard<decltype(m_lock)> guard{ m_lock };

            auto result = m_cache.emplace(name, std::weak_ptr<SharedMemory>{});
            if (!result.second)
            {
                if (auto memory = result.first->second.lock())
                {
                    return memory;
                }
            }

            std::shared_ptr<SharedMemory> memory;

            try
            {
                result.first->second = memory = std::make_shared<SharedMemory>(open_only, name);
            }
            catch (...)
            {
                m_cache.erase(result.first);
                throw;
            }

            Cleanup();

            return memory;
        }

    private:
        std::shared_ptr<SharedMemory> TryOpen(const char* name)
        {
            std::shared_lock<decltype(m_lock)> guard{ m_lock };

            auto it = m_cache.find(name);
            if (it != m_cache.end())
            {
                if (auto memory = it->second.lock())
                {
                    return memory;
                }
            }

            return{};
        }

        void Cleanup()
        {
            for (auto it = m_cache.begin(); it != m_cache.end(); )
            {
                if (it->second.expired())
                {
                    it = m_cache.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        std::unordered_map<std::string, std::weak_ptr<SharedMemory>> m_cache;
        std::shared_timed_mutex m_lock; // TODO: Use std::shared_mutex when available in VC14.
    };


    SharedMemoryCache::SharedMemoryCache()
        : m_impl{ std::make_unique<Impl>() }
    {}

    SharedMemoryCache::~SharedMemoryCache() = default;

    std::shared_ptr<SharedMemory> SharedMemoryCache::Create(const char* name, std::size_t size)
    {
        return m_impl->Create(name, size);
    }

    std::shared_ptr<SharedMemory> SharedMemoryCache::Open(const char* name)
    {
        return m_impl->Open(name);
    }

} // IPC
