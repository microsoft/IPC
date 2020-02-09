#pragma once

#include "Callback.h"
#include "IPC/Exception.h"
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>


namespace IPC
{
    namespace detail
    {
        template <typename Connector, typename TimeoutFactory, typename ErrorHandler, typename ComponentFactory, typename... TransactionArgs>
        auto Connect(
            const char* acceptorName,
            std::shared_ptr<Connector> connector,
            bool async,
            TimeoutFactory&& timeoutFactory,
            ErrorHandler&& errorHandler,
            ComponentFactory&& componentFactory,
            TransactionArgs&&... transactionArgs)
        {
            using Component = typename decltype(componentFactory(
                std::declval<std::unique_ptr<typename Connector::Connection>>(),
                std::declval<Callback<void()>>()))::element_type;                                           // Deduce the component type.

            using ComponentHolder = std::shared_ptr<Component>;                                             // Keep the component in shared_ptr.
            using State = std::pair<ComponentHolder, Callback<void(bool)>>;                                 // Store holder and a callback for reconnection.

            // This class will make sure that the user is still interested in reconnection and all objects are valid while we process it.
            struct Lifetime
                : public std::enable_shared_from_this<Lifetime>,
                  private std::weak_ptr<State>,                     // Only user keeps a strong reference. We will lock only while processing.
                  private std::mutex,                               // Mutex to use for below condition variable.
                  private std::condition_variable                   // A condition variable to wait on State expiration.
            {
                using WeakState = std::weak_ptr<State>;

                Lifetime(const std::shared_ptr<State>& state)
                    : WeakState{ state }
                {}

                // Tries to extend the current lifetime if not yet expired.
                // The returned object releases the extra reference counter and notifies any waiting thread.
                auto Extend()
                {
                    struct Notifier : private std::pair<std::shared_ptr<State>, std::shared_ptr<Lifetime>>
                    {
                        using std::pair<std::shared_ptr<State>, std::shared_ptr<Lifetime>>::pair;

                        Notifier(Notifier&& other) = default;

                        // Checks if we were able to extend.
                        explicit operator bool() const
                        {
                            return this->first != nullptr;
                        }

                        ~Notifier()
                        {
                            if (this->first)                    // Check if the content was moved out or it was already expired.
                            {
                                assert(this->second);
                                {
                                    std::lock_guard<std::mutex> guard{ *this->second };
                                    this->first.reset();        // Release the extra reference count.
                                }
                                this->second->notify_one();     // Notify any waiting thread about possible expiration.
                            }
                        }
                    };

                    return Notifier{ std::weak_ptr<State>::lock(), this->shared_from_this() };
                }

                // Blocks the current thread until any temporary extensions expire.
                void Wait()
                {
                    std::unique_lock<std::mutex> guard{ *this };
                    wait(guard, [this] { return this->expired(); });    // Block until the state expires.
                }
            };

            auto state = std::make_shared<State>();
            auto lifetime = std::make_shared<Lifetime>(state);
            auto& holder = state->first;
            auto& callback = state->second;

            const auto reconnector = [lifetime, &callback]  // Callback for recursive call to reconnection function.
            {
                if (auto ext = lifetime->Extend())          // Try extend the lifetime to make sure captured references are still valid.
                {
                    callback(true);                         // Pass 'true' to indicate asynchronous Connect call.
                }
            };

            using Retry = decltype(timeoutFactory(reconnector));

            callback =                                      // Set the reconnection function.
                [lifetime,                                  // Keep track of lifetime.
                    transactionArgs...,
                    &holder,                                // Capturing reference since it lives inside the state.
                    acceptorName = std::string{ acceptorName },
                    connector,
                    retry = std::make_shared<Retry>(timeoutFactory(reconnector)), // Make a timeout functor for the reconnection function.
                    errorHandler = std::forward<ErrorHandler>(errorHandler),
                    componentFactory = std::forward<ComponentFactory>(componentFactory)](bool async) mutable
                {
                    std::atomic_store(&holder, {}); // Mark that the connection is closed and not available.

                    // A function that will store the newly established connection in the holder.
                    auto store = [&](auto&& futureConnection) mutable // Capture references since they live inside the state.
                    {
                        try
                        {
                            // Construct a new component (Client or Server) for the new connection and pass retry as close handler.
                            std::atomic_store(
                                &holder,
                                ComponentHolder{ componentFactory(futureConnection.get(), [retry] { (*retry)(); }) });
                        }
                        catch (...)
                        {
                            errorHandler(std::current_exception()); // If something goes wrong, invoke user-defined error handler
                            (*retry)();                             // and retry.
                        }
                    };

                    
                    if (async)
                    {
                        connector->Connect(                             // Connect asynchronously.
                            acceptorName.c_str(),
                            [lifetime, store](auto&& futureConnection) mutable
                            {
                                if (auto ext = lifetime->Extend())      // Try extend the lifetime to make sure captured references are still valid.
                                {
                                    store(std::move(futureConnection)); // Try to construct a new component.
                                }
                            },
                            transactionArgs...);
                    }
                    else
                    {
                        store(connector->Connect(acceptorName.c_str(), transactionArgs...));// Connect synchronously and try to construct a new component.
                    }
                };

            callback(async);    // Start connection process.

            // Object that will keep the state alive and will block on destruction waiting on state expiration.
            // This is needed to make sure that when the user destroys the returned function the stored component is already destructed.
            std::shared_ptr<void> alive
                {
                    nullptr,
                    [state, lifetime, connector](void*) mutable // Keep the connector alive to prevent deadlock while destructing the state on separate thread.
                    {
                        state.reset();      // Release the state reference.
                        lifetime->Wait();   // Block current thread if there are any extensions running.
                    }
                };

            return [alive, &holder]         // Return a function that returns a component holder.
                {
                    if (auto component = std::atomic_load(&holder)) // Holder reference is valid as long as the alive object is valid.
                    {
                        return component;
                    }

                    throw Exception{ "Connection is not available." };
                };
        }

    } // detail
} // IPC
