#include "Timer.h"

namespace Spectrum {
    namespace Utils {

        Timer::Timer()
            : m_startTime(std::chrono::steady_clock::now()) {
        }

        void Timer::Reset() noexcept {
            m_startTime = std::chrono::steady_clock::now();
        }

        float Timer::GetElapsedSeconds() const noexcept {
            const auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<float>(now - m_startTime).count();
        }

        float Timer::GetElapsedMilliseconds() const noexcept {
            return GetElapsedSeconds() * 1000.0f;
        }

    } // namespace Utils
} // namespace Spectrum