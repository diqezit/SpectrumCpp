#ifndef SPECTRUM_CPP_COMMON_H
#define SPECTRUM_CPP_COMMON_H

// Windows headers
#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>

// Standard library headers
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <complex>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <random>
#include <optional>
#include <variant>
#include <type_traits>

// Link required libraries
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

// Include project types
#include "Types.h"

namespace wrl = Microsoft::WRL;

// Logging macros
#ifdef _DEBUG
#define LOG_DEBUG(msg)   std::cout << "[DEBUG] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg)   std::cerr << "[ERROR] " << msg << std::endl
#else
#define LOG_DEBUG(msg)
#define LOG_WARNING(msg)
#define LOG_ERROR(msg)
#endif
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Math
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    template<typename T>
    constexpr T Clamp(T v, T lo, T hi) noexcept { return std::clamp(v, lo, hi); }

    template<typename T>
    constexpr T Saturate(T v) noexcept { return Clamp(v, T(0), T(1)); }

    template<typename T>
    constexpr T Lerp(T a, T b, float t) noexcept { return a + (b - a) * t; }

    constexpr float DegreesToRadians(float d) noexcept { return d * DEG_TO_RAD; }
    constexpr float Normalized(float v) noexcept { return Saturate(v); }

    constexpr float Normalize(float v, float lo, float hi) noexcept {
        return Saturate((v - lo) / (hi - lo));
    }

    constexpr float Map(float v, float inLo, float inHi, float outLo, float outHi) noexcept {
        return Lerp(outLo, outHi, Normalize(v, inLo, inHi));
    }

    constexpr float SmoothStep(float e0, float e1, float x) noexcept {
        const float t = Normalize(x, e0, e1);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float WrapTwoPi(float a) noexcept {
        a = std::fmod(a, TWO_PI);
        if (a < 0.0f)
            a += TWO_PI;
        return a;
    }

    inline float SmoothDt(float factor, float dt, float fps = 60.0f) noexcept {
        return Clamp(factor * dt * fps, 0.0f, 1.0f);
    }

    inline float SmoothValue(
        float cur, float target, float attack = 0.4f, float decay = 0.85f) noexcept
    {
        float t = 1.0f - decay;
        if (cur < target)
            t = attack;
        return Lerp(cur, target, t);
    }

    inline void ResampleSpectrum(
        const SpectrumData& src, float* dst, size_t dstCount, float scale) noexcept
    {
        const size_t last = src.size() - 1;
        float step = 0.0f;
        if (dstCount > 1)
            step = float(last) / float(dstCount - 1);

        for (size_t i = 0; i < dstCount; ++i) {
            const float t = step * float(i);
            const size_t i0 = size_t(t);
            const size_t i1 = std::min(i0 + 1, last);
            dst[i] = Lerp(src[i0], src[i1], t - float(i0)) * scale;
        }
    }

} // namespace Spectrum

namespace Spectrum::Helpers::Utils {

    template<typename E>
    inline E CycleEnum(E cur, int dir) {
        using U = std::underlying_type_t<E>;
        const U n = U(E::Count);
        return E(((U(cur) + dir) % n + n) % n);
    }

    inline std::string_view ToString(FFTWindowType t) {
        constexpr std::string_view names[] = {
            "Rectangular", "Triangular", "Bartlett", "Cosine",
            "Hann", "Bartlett-Hann", "Hamming", "Bohman",
            "Blackman", "Blackman-Harris", "Kaiser", "Flat Top",
            "Gaussian", "Lanczos", "Cosine NP", "Planck Taper",
            "Tukey"
        };
        static_assert(
            sizeof(names) / sizeof(names[0]) == size_t(FFTWindowType::Count),
            "names must match FFTWindowType");
        return names[size_t(t)];
    }

    class Timer {
    public:
        void Reset() noexcept { m_t = Clock::now(); }

        float GetElapsedSeconds() const noexcept {
            return std::chrono::duration<float>(Clock::now() - m_t).count();
        }

    private:
        using Clock = std::chrono::steady_clock;
        Clock::time_point m_t{ Clock::now() };
    };

    class Random {
    public:
        static Random& Instance() {
            thread_local Random inst;
            return inst;
        }

        float Float(float lo = 0.0f, float hi = 1.0f) {
            return Lerp(lo, hi, m_unit(m_gen));
        }

    private:
        std::mt19937 m_gen{ std::random_device{}() };
        std::uniform_real_distribution<float> m_unit{ 0.0f, 1.0f };
    };

} // namespace Spectrum::Helpers::Utils

#endif // SPECTRUM_CPP_COMMON_H