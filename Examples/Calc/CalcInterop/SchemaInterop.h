#include "Schema.h"
#include <IPC/Managed/detail/Cast.h>
#include <IPC/Managed/detail/NativeObject.h>
#include <IPC/Managed/detail/Interop/SharedMemory.h>
#include <msclr/marshal.h>


namespace Calc
{
namespace Managed
{
    public enum class Operation
    {
        Add, Subtract, Multiply, Divide
    };

    [IPC::Managed::Object]
    public ref class Request
    {
    internal:
        Request(const Calc::Request& request)
        {
            X = request.X;
            Y = request.Y;
            Op = static_cast<Operation>(request.Op);
        }

    public:
        Request()
        {}

        property System::Single X;
        property System::Single Y;
        property Operation Op;
    };


    namespace
    {
#pragma managed(push, off)
        inline void EmplaceResponse(Calc::Response& response, IPC::SharedMemory& memory)
        {
            response.Text.emplace(memory.GetAllocator<char>());
        }

        inline void SetText(Calc::Response& response, const char* text)
        {
            response.Text->assign(text);
        }
#pragma managed(pop)
    }

    [IPC::Managed::Object]
    public ref class Response
    {
    internal:
        Response(const Calc::Response& response)
            : m_impl{ response }
        {}

    public:
        Response(IPC::Managed::SharedMemory^ memory)
        {
            EmplaceResponse(*m_impl, *memory->Impl);
        }

        property System::Single Z
        {
            System::Single get()
            {
                return m_impl->Z;
            }

            void set(System::Single value)
            {
                m_impl->Z = value;
            }
        }

        property System::String^ Text
        {
            System::String^ get()
            {
                return m_impl->Text ? msclr::interop::marshal_as<System::String^>(m_impl->Text->c_str()) : nullptr;
            }

            void set(System::String^ value)
            {
                SetText(*m_impl, value ? msclr::interop::marshal_context{}.marshal_as<const char*>(value) : "");
            }
        }

    internal:
        property Calc::Response& Impl
        {
            Calc::Response& get()
            {
                return *m_impl;
            }
        }

    private:
        IPC::Managed::detail::NativeObject<Calc::Response> m_impl;
    };
}
}
