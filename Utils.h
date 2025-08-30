// Utils.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Utils.h: Mathematical, color and general utility functions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_UTILS_H
#define SPECTRUM_CPP_UTILS_H

#include "Common.h"

#include <string>
#include <string_view>
#include <memory>
#include <random>
#include <chrono>

namespace Spectrum {
    namespace Utils {

        // Math utilities

        template<typename T>
        [[nodiscard]] inline T Clamp(T value, T minVal, T maxVal) noexcept {
            return value < minVal ? minVal : (value > maxVal ? maxVal : value);
        }

        template<typename T>
        [[nodiscard]] inline T Saturate(T value) noexcept {
            return Clamp<T>(value, static_cast<T>(0), static_cast<T>(1));
        }

        template<typename T>
        [[nodiscard]] inline T Lerp(T a, T b, float t) noexcept {
            return a + (b - a) * t;
        }

        [[nodiscard]] inline float Normalize(float value,
            float minVal,
            float maxVal) noexcept {
            const float denom = (maxVal - minVal);
            if (denom == 0.0f) return 0.0f;
            return (value - minVal) / denom;
        }

        [[nodiscard]] inline float Map(float value,
            float inMin,
            float inMax,
            float outMin,
            float outMax) noexcept {
            const float denom = (inMax - inMin);
            if (denom == 0.0f) return outMin;
            return outMin + (value - inMin) * (outMax - outMin) / denom;
        }

        [[nodiscard]] inline float SmoothStep(float edge0,
            float edge1,
            float x) noexcept {
            const float t = Saturate((x - edge0) / (edge1 - edge0));
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] inline float EaseInOut(float t) noexcept {
            return t < 0.5f ? 2.0f * t * t
                : -1.0f + (4.0f - 2.0f * t) * t;
        }

        [[nodiscard]] inline float DegToRad(float deg) noexcept {
            return deg * (PI / 180.0f);
        }

        [[nodiscard]] inline float RadToDeg(float rad) noexcept {
            return rad * (180.0f / PI);
        }

        [[nodiscard]] inline float FreqToMel(float freq) noexcept {
            return 2595.0f * std::log10(1.0f + freq / 700.0f);
        }

        [[nodiscard]] inline float MelToFreq(float mel) noexcept {
            return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
        }

        // Color utilities

        struct HSV {
            float h, s, v;
            HSV(float hIn = 0.0f, float sIn = 0.0f, float vIn = 0.0f)
                : h(hIn), s(sIn), v(vIn) {
            }
        };

        Color HSVtoRGB(const HSV& hsv);
        HSV   RGBtoHSV(const Color& rgb);

        uint32_t ColorToARGB(const Color& color);
        Color    ARGBtoColor(uint32_t argb);

        Color InterpolateColor(const Color& c1, const Color& c2, float t);
        Color AdjustBrightness(const Color& color, float factor);
        Color AdjustSaturation(const Color& color, float factor);

        // String utilities

        std::wstring StringToWString(const std::string& str);
        std::string  WStringToString(const std::wstring& wstr);

        template<typename... Args>
        [[nodiscard]] std::string Format(const std::string& fmt, Args... args) {
            int size = std::snprintf(nullptr, 0, fmt.c_str(), args...) + 1;
            if (size <= 0) return std::string();

            std::unique_ptr<char[]> buf(new char[static_cast<size_t>(size)]);
            std::snprintf(buf.get(), static_cast<size_t>(size), fmt.c_str(), args...);
            return std::string(buf.get(), buf.get() + (static_cast<size_t>(size) - 1));
        }

        // Window utilities

        [[nodiscard]] inline Point GetMousePosition(LPARAM lParam) noexcept {
            return Point(
                static_cast<float>(GET_X_LPARAM(lParam)),
                static_cast<float>(GET_Y_LPARAM(lParam))
            );
        }

        [[nodiscard]] inline bool IsKeyPressed(int vkCode) noexcept {
            return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        }

        // Time utilities

        class Timer {
        public:
            Timer();
            void  Reset() noexcept;
            float GetElapsedSeconds() const noexcept;
            float GetElapsedMilliseconds() const noexcept;

        private:
            std::chrono::steady_clock::time_point m_startTime;
        };

        // Random utilities

        class Random {
        public:
            static Random& Instance();

            float Float(float min = 0.0f, float max = 1.0f);
            int   Int(int min, int max);
            bool  Bool(float probability = 0.5f);

        private:
            Random();
            std::mt19937 m_generator;
            std::uniform_real_distribution<float> m_unitDist;
        };

    } // namespace Utils
} // namespace Spectrum

#endif // SPECTRUM_CPP_UTILS_H