#pragma once

#include <msclr/auto_handle.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        ref class ThreadDetector
        {
        internal:
            msclr::auto_handle<System::IDisposable> AutoDetect();

        public:
            System::IDisposable^ Detect();

            property bool IsExpectedThread
            {
                bool get();
            }

        private:
            ref class Detector;

            System::Threading::ThreadLocal<bool> m_isExpected;
        };

    } // detail

} // Managed
} // IPC
