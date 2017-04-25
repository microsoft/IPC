#pragma once

#pragma warning(push)
#pragma warning(disable : 4459)
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/weak_ptr.hpp>
#pragma warning(pop)

#include "detail/Alias.h"
#include "detail/SpinLock.h"
#include "detail/RecursiveSpinLock.h"
#include "Exception.h"
#include <string>
#include <type_traits>


namespace IPC
{
    class SharedMemory
    {
        struct MutexFamily
        {
            using mutex_type = detail::SpinLock;
            using recursive_mutex_type = detail::RecursiveSpinLock;
        };

        using ManagedSharedMemory = detail::ipc::basic_managed_windows_shared_memory<
            char, detail::ipc::rbtree_best_fit<MutexFamily>, detail::ipc::iset_index>;

    public:
        using Handle = ManagedSharedMemory::handle_t;

        template <typename T>
        class Allocator : public ManagedSharedMemory::allocator<T>::type
        {
            using Base = typename ManagedSharedMemory::allocator<T>::type;

        public:
            template <typename U>
            struct rebind
            {
                using other = Allocator<U>;
            };

            using Base::Base;

            using Base::construct;

            void construct(const typename Base::pointer& ptr, const boost::container::default_init_t&)
            {
                ::new (static_cast<void*>(boost::interprocess::ipcdetail::to_raw_pointer(ptr)), boost_container_new_t()) typename Base::value_type;
            }
        };

        template <typename T>
        using Deleter = typename ManagedSharedMemory::deleter<T>::type;

        template <typename T>
        using UniquePtr = typename detail::ipc::managed_unique_ptr<T, ManagedSharedMemory>::type;

        template <typename T>
        using SharedPtr = typename detail::ipc::managed_shared_ptr<T, ManagedSharedMemory>::type;

        template <typename T>
        using WeakPtr = typename detail::ipc::managed_weak_ptr<T, ManagedSharedMemory>::type;

        SharedMemory(create_only_t, const char* name, std::size_t size);

        SharedMemory(open_only_t, const char* name);

        SharedMemory(const SharedMemory& other) = delete;
        SharedMemory& operator=(const SharedMemory& other) = delete;

        SharedMemory(SharedMemory&& other) = default;
        SharedMemory& operator=(SharedMemory&& other) = default;

        template <typename T>
        Allocator<T> GetAllocator()
        {
            return{ m_memory.get_allocator<T>() };
        }

        template <typename T>
        Deleter<T> GetDeleter()
        {
            return m_memory.get_deleter<T>();
        }

        template <typename T, typename Name, typename... Args>
        T& Construct(Name name, Args&&... args)
        {
            static_assert(IsValidName<Name>::value, "Unknown name or name tag is used.");

            return *m_memory.construct<T>(name)(std::forward<Args>(args)...);
        }

        template <typename T, typename Name>
        T& Find(Name name)
        {
            static_assert(IsValidName<Name>::value, "Unknown name or name tag is used.");

            if (auto obj = m_memory.find<T>(name).first)
            {
                return *obj;
            }

            throw Exception{ "Unable to find the object." };
        }

        template <typename T>
        void Destruct(T* obj)
        {
            assert(obj == nullptr || m_memory.belongs_to_segment(obj));
            m_memory.destroy_ptr(obj);
        }

        template <typename T, typename Name, typename... Args>
        UniquePtr<T> MakeUnique(Name name, Args&&... args)
        {
            static_assert(IsValidName<Name>::value, "Unknown name or name tag is used.");

            return MakeUniquePtr(&Construct<T>(name, std::forward<Args>(args)...));
        }

        template <typename T>
        UniquePtr<T> MakeUniquePtr(T* obj)
        {
            assert(obj == nullptr || m_memory.belongs_to_segment(obj));
            return detail::ipc::make_managed_unique_ptr(obj, m_memory);
        }

        template <typename T, typename Name, typename... Args>
        SharedPtr<T> MakeShared(Name name, Args&&... args)
        {
            static_assert(IsValidName<Name>::value, "Unknown name or name tag is used.");

            return MakeSharedPtr(&Construct<T>(name, std::forward<Args>(args)...));
        }

        template <typename T>
        SharedPtr<T> MakeSharedPtr(T* obj)
        {
            assert(obj == nullptr || Contains(obj));
            return detail::ipc::make_managed_shared_ptr(obj, m_memory);
        }

        template <typename T>
        WeakPtr<T> MakeWeakPtr(const SharedPtr<T>& obj)
        {
            return typename detail::ipc::managed_weak_ptr<T, ManagedSharedMemory>::type{ obj };
        }

        template <typename T>
        Handle ToHandle(T& obj) const
        {
            assert(Contains(&obj));
            return m_memory.get_handle_from_address(&obj);
        }

        template <typename T>
        T& FromHandle(Handle handle) const
        {
            auto ptr = m_memory.get_address_from_handle(handle);
            assert(Contains(ptr));
            return *static_cast<T*>(ptr);
        }

        template <typename Function>
        void InvokeAtomic(Function&& func)
        {
            m_memory.atomic_func(func);
        }

        bool Contains(const void* ptr) const;

        const std::string& GetName() const;

        std::size_t GetFreeSize() const;

        static std::size_t GetMinSize();

    private:
        template <typename Name>
        using IsValidName = std::bool_constant<
            std::is_same<std::decay_t<Name>, const char*>::value
            || std::is_same<std::decay_t<Name>, decltype(anonymous_instance)>::value
            || std::is_same<std::decay_t<Name>, decltype(unique_instance)>::value>;

        ManagedSharedMemory m_memory;
        std::string m_name;
    };

} // IPC
