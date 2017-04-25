#pragma once

#include <memory>
#include <exception>


namespace IPC
{
    namespace detail
    {
        template <typename Acceptor, typename Collection, typename ComponentFactory, typename ErrorHandler, typename... Args>
        auto Accept(const char* name, std::shared_ptr<Collection> components, ComponentFactory&& componentFactory, ErrorHandler&& errorHandler, Args&&... args)
        {
            auto acceptor = std::make_shared<Acceptor>(
                name,
                [components, componentFactory = std::forward<ComponentFactory>(componentFactory), errorHandler = std::forward<ErrorHandler>(errorHandler)](auto&& futureConnection) mutable
                {
                    try
                    {
                        components->Accept(
                            [&](auto&& closeHandler)
                            {
                                return componentFactory(futureConnection.get(), std::forward<decltype(closeHandler)>(closeHandler));
                            });
                    }
                    catch (...)
                    {
                        errorHandler(std::current_exception());
                    }
                },
                std::forward<Args>(args)...);

            return [components = std::move(components), acceptor = std::move(acceptor)] { return components->GetComponents(); };
        }

    } // detail
} // IPC
