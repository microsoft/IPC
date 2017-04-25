#include "IPC/Managed/detail/Throw.h"
#include "Exception.h"

#pragma managed(push, off)
#include "IPC/Exception.h"
#include <boost/interprocess/exceptions.hpp>
#include <stdexcept>
#pragma managed(pop)


namespace IPC
{
namespace Managed
{
    namespace detail
    {
#pragma managed(push, off)
        namespace
        {
            [[noreturn]] void RethrowException(std::exception_ptr error)
            {
                std::rethrow_exception(error);
            }
        }
#pragma managed(pop)

        void ThrowManagedException(std::exception_ptr error)
        {
            try
            {
                RethrowException(error);  // TODO: Support nested exceptions.
            }
            catch (const IPC::Exception& e)
            {
                throw gcnew Managed::Exception{ e };
            }
            catch (const boost::interprocess::bad_alloc& e)
            {
                throw gcnew System::OutOfMemoryException{ gcnew System::String{ e.what() } };
            }
            catch (const boost::interprocess::interprocess_exception& e)
            {
                throw gcnew Managed::Exception{ e };
            }
            catch (const std::invalid_argument& e)
            {
                throw gcnew System::ArgumentException{ gcnew System::String{ e.what() } };
            }
            catch (const std::out_of_range& e)
            {
                throw gcnew System::ArgumentOutOfRangeException{ gcnew System::String{ e.what() } };
            }
            catch (const std::bad_cast& e)
            {
                throw gcnew System::InvalidCastException{ gcnew System::String{ e.what() } };
            }
            catch (const std::bad_alloc& e)
            {
                throw gcnew System::OutOfMemoryException{ gcnew System::String{ e.what() } };
            }
            catch (const std::exception& e)
            {
                throw gcnew System::Exception{ gcnew System::String{ e.what() } };
            }
        }

    } // detail

} // Managed
} // IPC
