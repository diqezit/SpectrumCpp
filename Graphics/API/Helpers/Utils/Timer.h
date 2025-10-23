#ifndef SPECTRUM_CPP_TIMER_H
#define SPECTRUM_CPP_TIMER_H

#include "Common/Common.h"
#include <chrono>

namespace Spectrum::Helpers::Utils {

    class Timer {
    public:
        Timer();
        void  Reset() noexcept;
        float GetElapsedSeconds() const noexcept;
        float GetElapsedMilliseconds() const noexcept;

    private:
        std::chrono::steady_clock::time_point m_startTime;
    };

} // namespace Spectrum::Helpers::Utils

#endif // SPECTRUM_CPP_TIMER_H