#include "stdafx.h"
#include "WaitHandleFactoryMock.h"
#include <vector>
#include <cassert>


namespace IPC
{
namespace UnitTest
{
namespace Mocks
{
    WaitHandleFactory::WaitHandleFactory()
        : shared_ptr{ std::make_shared<element_type>() },
          m_rd{ std::make_shared<std::random_device>() }
    {}

    std::size_t WaitHandleFactory::Process()
    {
        std::size_t count{ 0 };

        while (auto n = ProcessSinglePass())
        {
            count += n;
        }

        return count;
    }

    std::shared_ptr<void> WaitHandleFactory::operator()(detail::KernelObject obj, detail::Callback<bool()> callback) const
    {
        assert(obj);
        assert(callback);

        auto handler = std::make_shared<decltype(callback)>(std::move(callback));

        return{ get()->emplace(std::move(obj), handler).second ? handler : nullptr };
    }

    std::size_t WaitHandleFactory::ProcessSinglePass()
    {
        std::vector<element_type::iterator> items;
        items.reserve(get()->size());

        for (auto it = get()->begin(); it != get()->end(); ++it)
        {
            items.push_back(it);
        }

        std::shuffle(items.begin(), items.end(), std::mt19937{ (*m_rd)() });

        std::size_t count{ 0 };

        for (auto it : items)
        {
            auto handler = it->second.lock();
            auto isSignaled = [&] { return it->first.IsSignaled(); };

            if (!handler || (isSignaled() && !(++count, *handler)()))
            {
                get()->erase(it);
            }
        }

        return count;
    }


} // Mocks
} // UnitTest
} // IPC
