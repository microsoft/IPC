#pragma once

#include "detail/ChannelBase.h"
#include "Exception.h"
#include <memory>


namespace IPC
{
    struct DefaultTraits;


    template <typename T, typename Traits = DefaultTraits>
    class OutputChannel : public detail::ChannelBase<T, Traits::template Queue>
    {
        using ChannelBase = detail::ChannelBase<T, Traits::template Queue>;

    public:
        using ChannelBase::ChannelBase;

        template <typename U>
        bool TrySend(U&& value)
        {
            if (this->GetQueue().Push(std::forward<U>(value)))
            {
                if (++this->GetCounter() == 1)
                {
                    this->GetNotEmptyEvent().Signal();
                }

                return true;
            }

            return false;
        }

        template <typename U>
        void Send(U&& value)
        {
            if (!TrySend(std::forward<U>(value)))
            {
                throw Exception{ "Out of buffers." };
            }
        }
    };

} // IPC
