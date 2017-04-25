#pragma once

#include "detail/ThreadDetector.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        ref class ThreadDetector::Detector
        {
        public:
            Detector(ThreadDetector^ detector)
                : m_detector{ detector }
            {
                if (m_detector->m_isExpected.Value)
                {
                    m_prev = true;
                }
                else
                {
                    m_prev = false;
                    m_detector->m_isExpected.Value = true;
                }
            }

            ~Detector()
            {
                if (!m_prev)
                {
                    m_detector->m_isExpected.Value = false;
                }
            }

        private:
            ThreadDetector^ m_detector;
            bool m_prev;
        };


        msclr::auto_handle<System::IDisposable> ThreadDetector::AutoDetect()
        {
            return msclr::auto_handle<System::IDisposable>{ Detect() };
        }

        System::IDisposable^ ThreadDetector::Detect()
        {
            return gcnew Detector{ this };
        }

        bool ThreadDetector::IsExpectedThread::get()
        {
            return m_isExpected.Value;
        }

    } // detail

} // Managed
} // IPC
