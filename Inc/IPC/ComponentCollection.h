#pragma once

#include <vector>
#include <list>
#include <memory>
#include <shared_mutex>


namespace IPC
{
    namespace detail
    {
        template <typename T>
        struct IsStable : std::false_type
        {};

        template <typename T, typename Enable = void>
        class ComponentCollectionInserter;

        template <typename T>
        class ComponentCollectionInserter<T, std::enable_if_t<!IsStable<T>::value>>
        {
            static_assert(IsStable<T>::value, "Only stable containers are allowed.");
        };

    } // detail


    template <typename Container>
    class ComponentCollection
    {
    public:
        ComponentCollection()
            : m_components{ std::make_shared<LockableContainer>() },
              m_inserter{ *m_components }
        {}

        ComponentCollection(const ComponentCollection& other) = delete;
        ComponentCollection& operator=(const ComponentCollection& other) = delete;

        ComponentCollection(ComponentCollection&& other) = default;
        ComponentCollection& operator=(ComponentCollection&& other) = default;

        ~ComponentCollection()
        {
            if (m_components)
            {
                std::vector<typename Container::value_type> components;
                auto newContainer = std::make_shared<LockableContainer>();
                {
                    std::lock_guard<std::shared_timed_mutex> guard{ *m_components };

                    std::copy(
                        std::make_move_iterator(std::begin(*m_components)),
                        std::make_move_iterator(std::end(*m_components)),
                        std::back_inserter(components));

                    m_components.swap(newContainer);
                }
            }
        }

        explicit operator bool() const
        {
            return static_cast<bool>(m_components);
        }

        template <typename ComponentFactory>
        bool Accept(ComponentFactory&& componentFactory)
        {
            struct Deleter
            {
                void operator()(void*) const
                {
                    if (auto components = m_components.lock())
                    {
                        std::lock_guard<std::shared_timed_mutex> guard{ *components };
                        components->erase(m_location);
                    }
                }

                std::weak_ptr<LockableContainer> m_components;
                typename Container::iterator m_location{};
            };

            std::shared_ptr<void> eraser{ nullptr, Deleter{} };

            auto component = componentFactory([eraser]() mutable { eraser.reset(); });

            if (!eraser.unique())
            {
                auto deleter = std::get_deleter<Deleter>(eraser);
                assert(deleter);
                {
                    std::lock_guard<std::shared_timed_mutex> guard{ *m_components };

                    auto location = m_inserter(std::move(component));

                    if (location != m_components->end())    // Otherwise leave the deleter->m_components empty so that Deleter becomes a no-op.
                    {
                        deleter->m_location = location;
                        deleter->m_components = m_components;
                        return true;
                    }
                }
            }

            return false;
        }

        auto GetComponents() const
        {
            class Accessor : private std::shared_lock<std::shared_timed_mutex>
            {
            public:
                explicit Accessor(std::shared_ptr<LockableContainer> components)
                    : shared_lock{ *components },
                      m_components{ std::move(components) }
                {}

                const Container& operator*() const
                {
                    return *m_components;
                }

                const Container* operator->() const
                {
                    return m_components.get();
                }

            private:
                std::shared_ptr<LockableContainer> m_components;
            };

            return Accessor{ m_components };
        }

    private:
        struct LockableContainer : Container, std::shared_timed_mutex   // TODO: Use std::shared_mutex when available in VC14.
        {
            LockableContainer() = default;
        };

        std::shared_ptr<LockableContainer> m_components;
        detail::ComponentCollectionInserter<Container> m_inserter;
    };


    namespace detail
    {
        template <typename T, typename Allocator>
        struct IsStable<std::list<T, Allocator>> : std::true_type
        {};


        template <typename T, typename Allocator>
        class ComponentCollectionInserter<std::list<T, Allocator>>
        {
        public:
            explicit ComponentCollectionInserter(std::list<T, Allocator>& container)
                : m_container{ &container }
            {}

            template <typename U>
            auto operator()(U&& value)
            {
                return m_container->insert(m_container->end(), std::forward<U>(value));
            }

        private:
            std::list<T, Allocator>* m_container;
        };

    } // detail


    template <typename Server>
    class ServerCollection : public ComponentCollection<std::list<std::shared_ptr<Server>>>
    {
        using Base = ComponentCollection<std::list<std::shared_ptr<Server>>>;

    public:
        using Base::Accept;

        template <typename Handler>
        bool Accept(std::unique_ptr<typename Server::Connection> connection, Handler&& handler)
        {
            return Base::Accept(
                [&](auto&& closeHandler)
                {
                    return std::make_shared<Server>(std::move(connection), std::forward<Handler>(handler), std::forward<decltype(closeHandler)>(closeHandler));
                });
        }
    };


    template <typename Client>
    class ClientCollection : public ComponentCollection<std::list<std::shared_ptr<Client>>>
    {
        using Base = ComponentCollection<std::list<std::shared_ptr<Client>>>;

    public:
        using Base::Accept;

        bool Accept(std::unique_ptr<typename Client::Connection> connection)
        {
            return Base::Accept(
                [&](auto&& closeHandler)
                {
                    return std::make_shared<Client>(std::move(connection), std::forward<decltype(closeHandler)>(closeHandler));
                });
        }
    };

} // IPC
