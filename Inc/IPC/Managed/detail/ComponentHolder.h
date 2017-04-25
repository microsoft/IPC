#pragma once

#include "NativeObject.h"
#include "ManagedCallback.h"

#pragma managed(push, off)
#include "IPC/Exception.h"
#include <memory>
#include <functional>
#pragma managed(pop)

#include <msclr/gcroot.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <typename T>
        ref struct SelfDisposer : NativeObject<T>
        {
            explicit SelfDisposer(const T& obj)
                : NativeObject{ obj }
            {}

            void operator()(void*)
            {
                delete this;
            }
        };


        template <typename T>
        class ComponentHolder : public std::function<void(void*)>
        {
        public:
            ComponentHolder(T obj)
                : ComponentHolder{ std::shared_ptr<msclr::gcroot<T>>{ new msclr::gcroot<T>{ obj } } }
            {}

            operator T()
            {
                if (auto obj = m_obj.lock())
                {
                    return *obj;
                }

                throw IPC::Exception{ "Connection is not available." };
            }

        private:
            ComponentHolder(const std::shared_ptr<msclr::gcroot<T>>& obj)
                : function{ MakeManagedCallback(gcnew SelfDisposer<std::shared_ptr<msclr::gcroot<T>>>{ obj }) },
                  m_obj{ obj }
            {}

            std::weak_ptr<msclr::gcroot<T>> m_obj;
        };

    } // detail

} // Managed
} // IPC
