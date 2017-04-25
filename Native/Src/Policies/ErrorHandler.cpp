#include "stdafx.h"
#include "IPC/Policies/ErrorHandler.h"


namespace IPC
{
namespace Policies
{
    ErrorHandler::ErrorHandler(std::ostream& stream)
        : m_stream{ stream }
    {}

    void ErrorHandler::operator()(std::exception_ptr error) const
    {
        m_stream << "IPC: ";

        try
        {
            std::rethrow_exception(std::move(error));
        }
        catch (const std::exception& e)
        {
            m_stream << e.what();
        }
        catch (...)
        {
            m_stream << "Unknown exception.";
        }

        m_stream << std::endl;
    };

} // Policies
} // IPC
