#pragma once


namespace IPC
{
namespace Containers
{
namespace Managed
{
    generic <typename T>
    [IPC::Managed::Object]
    public interface struct IVector : System::IDisposable, System::Collections::Generic::IList<T>
    {
        property System::Int32 Capacity
        {
            System::Int32 get();
        }

        void Reserve(System::Int32 size);

        void Resize(System::Int32 size, T value);
    };

} // Managed
} // Containers
} // IPC
