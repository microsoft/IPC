#pragma once

#include "ConnectionFwd.h"
#include "detail/ConnectionBase.h"
#include <memory>
#include <type_traits>


namespace IPC
{
    struct DefaultTraits;


    template <typename Input, typename Output, typename Traits>
    class Connection : public detail::ConnectionBase
    {
        using ChannelHolder = detail::ChannelHolderOf<Input, Output, Traits>;

    public:
        template <typename... Channels>
        Connection(
            detail::KernelEvent localCloseEvent,
            detail::KernelEvent remoteCloseEvent,
            detail::KernelObject process,
            typename Traits::WaitHandleFactory waitHandleFactory,
            Channels&&... channels)
            : Connection{
                std::move(localCloseEvent),
                false,
                std::move(remoteCloseEvent),
                std::move(process),
                std::move(waitHandleFactory),
                std::forward<Channels>(channels)... }
        {}

        template <typename... Channels>
        Connection(
            detail::KernelEvent localCloseEvent,
            bool monitorLocalCloseEvent,
            detail::KernelEvent remoteCloseEvent,
            detail::KernelObject process,
            typename Traits::WaitHandleFactory waitHandleFactory,
            Channels&&... channels)
            : Connection{
                ChannelHolder{ std::forward<Channels>(channels)... },
                std::move(localCloseEvent),
                monitorLocalCloseEvent,
                std::move(remoteCloseEvent),
                std::move(process),
                std::move(waitHandleFactory) }
        {}

        ~Connection()
        {
            Close();
        }

        template <typename = std::enable_if_t<!std::is_void<Input>::value>>
        auto GetInputChannel(const std::nothrow_t&)
        {
            return IsClosed() ? nullptr : &std::get<InputChannel<Input, Traits>>(m_channels);
        }

        template <typename = std::enable_if_t<!std::is_void<Input>::value>>
        decltype(auto) GetInputChannel()
        {
            ThrowIfClosed();
            return std::get<InputChannel<Input, Traits>>(m_channels);
        }

        template <typename = std::enable_if_t<!std::is_void<Input>::value>>
        decltype(auto) GetInputChannel() const
        {
            ThrowIfClosed();
            return std::get<InputChannel<Input, Traits>>(m_channels);
        }

        template <typename = std::enable_if_t<!std::is_void<Output>::value>>
        auto GetOutputChannel(const std::nothrow_t&)
        {
            return IsClosed() ? nullptr : &std::get<OutputChannel<Output, Traits>>(m_channels);
        }

        template <typename = std::enable_if_t<!std::is_void<Output>::value>>
        decltype(auto) GetOutputChannel()
        {
            ThrowIfClosed();
            return std::get<OutputChannel<Output, Traits>>(m_channels);
        }

        template <typename = std::enable_if_t<!std::is_void<Output>::value>>
        decltype(auto) GetOutputChannel() const
        {
            ThrowIfClosed();
            return std::get<OutputChannel<Output, Traits>>(m_channels);
        }

        template <typename I = Input, std::enable_if_t<!std::is_void<I>::value>* = nullptr>
        void Close()
        {
            std::get<InputChannel<Input, Traits>>(m_channels).UnregisterReceiver();
            Close<void>();
        }

        template <typename I = Input, std::enable_if_t<std::is_void<I>::value>* = nullptr>
        void Close()
        {
            m_localCloseMonitor = {};
            m_remoteCloseMonitor = {};
            m_processMonitor = {};

            ConnectionBase::Close();
        }

    private:
        Connection(
            ChannelHolder&& channels,
            detail::KernelEvent&& localCloseEvent,
            bool monitorLocalCloseEvent,
            detail::KernelEvent&& remoteCloseEvent,
            detail::KernelObject&& process,
            typename Traits::WaitHandleFactory&& waitHandleFactory)
            : ConnectionBase{ localCloseEvent },
              m_channels{ std::move(channels) },
              m_waitHandleFactory{ std::move(waitHandleFactory) },
              m_localCloseMonitor{
                    monitorLocalCloseEvent && localCloseEvent
                    ? m_waitHandleFactory(std::move(localCloseEvent), [this] { CloseHandler(); return false; })
                    : WaitHandle{} },
              m_remoteCloseMonitor{ m_waitHandleFactory(std::move(remoteCloseEvent), [this] { CloseHandler(); return false; }) },
              m_processMonitor{ m_waitHandleFactory(std::move(process), [this] { CloseHandler(); return false; }) }
        {}

        using WaitHandle = decltype(std::declval<typename Traits::WaitHandleFactory>()(
            std::declval<detail::KernelObject>(), std::declval<bool(*)()>()));

        ChannelHolder m_channels;
        typename Traits::WaitHandleFactory m_waitHandleFactory;
        WaitHandle m_localCloseMonitor;
        WaitHandle m_remoteCloseMonitor;
        WaitHandle m_processMonitor;
    };

} // IPC
