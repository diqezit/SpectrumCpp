#ifndef SPECTRUM_CPP_MATH_UTILS_H
#define SPECTRUM_CPP_MATH_UTILS_H

#include "Common.h"

namespace Spectrum {
    namespace Utils {

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

        [[nodiscard]] inline float Normalize(
            float value,
            float minVal,
            float maxVal
        ) noexcept {
            const float denom = (maxVal - minVal);
            if (denom == 0.0f) {
                return 0.0f;
            }
            return (value - minVal) / denom;
        }

        [[nodiscard]] inline float Map(
            float value,
            float inMin,
            float inMax,
            float outMin,
            float outMax
        ) noexcept {
            const float denom = (inMax - inMin);
            if (denom == 0.0f) {
                return outMin;
            }
            return outMin + (value - inMin) * (outMax - outMin) / denom;
        }

        [[nodiscard]] inline float SmoothStep(
            float edge0,
            float edge1,
            float x
        ) noexcept {
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

    } // namespace Utils
} // namespace Spectrum

#endif