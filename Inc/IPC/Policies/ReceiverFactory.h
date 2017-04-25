#pragma once

#include "ReceiverFactoryFwd.h"
#include "InlineReceiverFactory.h"
#include "AsyncReceiverFactory.h"
#include <type_traits>

#pragma warning(push)
#include <boost/variant.hpp>
#pragma warning(pop)


namespace IPC
{
    namespace detail
    {
        template <typename... Factories>
        class VariantReceiverFactory
        {
        public:
            VariantReceiverFactory() = default;

            template <typename Factory,
                typename = std::enable_if_t<!std::is_base_of<VariantReceiverFactory, std::decay_t<Factory>>::value>>
            VariantReceiverFactory(Factory&& factory)
                : m_factory{ std::forward<Factory>(factory) }
            {}

            VariantReceiverFactory(const VariantReceiverFactory& other) = default;    // TODO: Remove this when crashing bugs are fixed in VC14.

            template <typename Queue, typename Handler>
            auto operator()(Queue& queue, Handler&& handler) const
            {
                using Receiver = boost::variant<decltype(std::declval<Factories>()(queue, std::forward<Handler>(handler)))...>;

                auto&& receiver = boost::apply_visitor(
                    [&](auto& factory) { return Receiver{ factory(queue, std::forward<Handler>(handler)) }; },
                    m_factory);

                return [receiver = std::make_unique<Receiver>(std::move(receiver))] // TODO: Do not make_unique when VC14 bugs are fixed.
                {
                    return boost::apply_visitor([](auto& r) { return r(); }, *receiver);
                };
            }

        private:
            boost::variant<Factories...> m_factory;
        };

    } // detail
} // IPC
