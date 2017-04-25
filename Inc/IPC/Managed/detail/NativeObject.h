#pragma once

#include "Interop/ExternalConstructor.h"
#include "Throw.h"

#pragma managed(push, off)
#include <typeinfo>
#include <type_traits>
#pragma managed(pop)


namespace IPC
{
namespace Managed
{
    namespace detail
    {
#pragma managed(push, off)
        template <typename T, typename... Args, std::enable_if_t<Interop::ExternalConstructor<T>::value>* = nullptr>
        T* New(Args&&... args)
        {
            return Interop::ExternalConstructor<T>::New(std::forward<Args>(args)...);
        }

        template <typename T, typename... Args, std::enable_if_t<!Interop::ExternalConstructor<T>::value>* = nullptr>
        T* New(Args&&... args)
        {
            return new T(std::forward<Args>(args)...);
        }

        template <typename T, std::enable_if_t<Interop::ExternalConstructor<T>::value>* = nullptr>
        void Delete(T* obj)
        {
            Interop::ExternalConstructor<T>::Delete(obj);
        }

        template <typename T, std::enable_if_t<!Interop::ExternalConstructor<T>::value>* = nullptr>
        void Delete(T* obj)
        {
            delete obj;
        }
#pragma managed(pop)


        template <typename T>
        ref class NativeObject
        {
        public:
            template <typename... Args>
            explicit NativeObject(Args&&... args)
            try
                : m_obj{ New<T>(std::forward<Args>(args)...) }
            {}
            catch (const std::exception& /*e*/)
            {
                ThrowManagedException(std::current_exception());
            }

            ~NativeObject()
            {
                this->!NativeObject();
            }

            !NativeObject()
            {
                Delete(m_obj);
                m_obj = nullptr;
            }

            explicit operator bool()
            {
                return m_obj != nullptr;
            }

            static T& operator*(NativeObject% obj)
            {
                return obj.Object;
            }

            static T* operator->(NativeObject% obj)
            {
                return &obj.Object;
            }

        private:
            property T& Object
            {
                T& get()
                {
                    if (!m_obj)
                    {
                        throw gcnew System::ObjectDisposedException{ gcnew System::String{ typeid(T).name() } };
                    }

                    return *m_obj;
                }
            }

            T* m_obj;
        };

    } // detail

} // Managed
} // IPC
