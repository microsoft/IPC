#pragma once

#include "IPC/InputChannel.h"
#include "IPC/OutputChannel.h"
#include "KernelEvent.h"
#include "Callback.h"
#include <tuple>
#include <memory>
#include <atomic>
#include <mutex>


namespace IPC
{
    namespace detail
    {
        template <typename Input, typename Output, typename Traits>
        struct ChannelHolder
        {
            using type = std::tuple<InputChannel<Input, Traits>, OutputChannel<Output, Traits>>;
        };

        template <typename Input, typename Traits>
        struct ChannelHolder<Input, void, Traits>
        {
            using type = std::tuple<InputChannel<Input, Traits>>;
        };

        template <typename Output, typename Traits>
        struct ChannelHolder<void, Output, Traits>
        {
            using type = std::tuple<OutputChannel<Output, Traits>>;
        };


        template <typename Input, typename Output, typename Traits>
        using ChannelHolderOf = typename ChannelHolder<Input, Output, Traits>::type;


        class ConnectionBase
        {
        public:
            ConnectionBase(const ConnectionBase& other) = delete;
            ConnectionBase& operator=(const ConnectionBase& other) = delete;

            virtual ~ConnectionBase();

            bool IsClosed() const;

            void Close();

            bool RegisterCloseHandler(Callback<void()> handler, bool combine = false);

            bool UnregisterCloseHandler();

        protected:
            explicit ConnectionBase(KernelEvent localCloseEvent);

            void ThrowIfClosed() const;

            void CloseHandler();

        private:
            std::atomic_bool m_isClosed{ false };
            KernelEvent m_closeEvent;
            std::mutex m_closeHandlerMutex;
            Callback<void()> m_closeHandler;
        };

    } // detail
} // IPC
