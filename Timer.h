#ifndef SPECTRUM_CPP_TIMER_H
#define SPECTRUM_CPP_TIMER_H

#include "Common.h"
#include <chrono>

namespace Spectrum {
    namespace Utils {

        class Timer {
        public:
            Timer();
            void  Reset() noexcept;
            float GetElapsedSeconds() const noexcept;
            float GetElapsedMilliseconds() const noexcept;

        private:
            std::chrono::steady_clock::time_point m_startTime;
        };

    } // namespace Utils
} // namespace Spectrum

#endif