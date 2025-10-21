#ifndef SPECTRUM_CPP_MATH_UTILS_H
#define SPECTRUM_CPP_MATH_UTILS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines mathematical utility functions for the Spectrum application,
// including interpolation, easing, normalization, and domain-specific
// transformations (frequency/mel scale conversions).
//
// This header provides a comprehensive suite of easing functions based on
// Robert Penner's equations, as well as spring damping and exponential
// smoothing for frame-rate independent animations.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <algorithm>

namespace Spectrum {
    namespace Utils {

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Basic Math Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        template<typename T>
        [[nodiscard]] constexpr T Clamp(T value, T minVal, T maxVal) noexcept {
            return std::clamp(value, minVal, maxVal);
        }

        template<typename T>
        [[nodiscard]] constexpr T Saturate(T value) noexcept {
            return std::clamp(value, static_cast<T>(0), static_cast<T>(1));
        }

        template<typename T>
        [[nodiscard]] constexpr T Lerp(T a, T b, float t) noexcept {
            return a + (b - a) * t;
        }

        [[nodiscard]] constexpr float Normalize(
            float value,
            float minVal,
            float maxVal
        ) noexcept {
            const float denom = (maxVal - minVal);
            if (denom == 0.0f) return 0.0f;
            return (value - minVal) / denom;
        }

        [[nodiscard]] constexpr float Map(
            float value,
            float inMin,
            float inMax,
            float outMin,
            float outMax
        ) noexcept {
            const float denom = (inMax - inMin);
            if (denom == 0.0f) return outMin;
            return outMin + (value - inMin) * (outMax - outMin) / denom;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Easing Functions (Robert Penner's Equations)
        // All functions take t in [0, 1] and return value in [0, 1]
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] constexpr float EaseLinear(float t) noexcept {
            return t;
        }

        [[nodiscard]] constexpr float EaseInQuad(float t) noexcept {
            return t * t;
        }

        [[nodiscard]] constexpr float EaseOutQuad(float t) noexcept {
            return t * (2.0f - t);
        }

        [[nodiscard]] constexpr float EaseInOutQuad(float t) noexcept {
            return t < 0.5f
                ? 2.0f * t * t
                : -1.0f + (4.0f - 2.0f * t) * t;
        }

        [[nodiscard]] constexpr float EaseInCubic(float t) noexcept {
            return t * t * t;
        }

        [[nodiscard]] constexpr float EaseOutCubic(float t) noexcept {
            const float f = t - 1.0f;
            return f * f * f + 1.0f;
        }

        [[nodiscard]] constexpr float EaseInOutCubic(float t) noexcept {
            return t < 0.5f
                ? 4.0f * t * t * t
                : 1.0f + (--t) * (2.0f * t) * (2.0f * t);
        }

        [[nodiscard]] constexpr float EaseInQuart(float t) noexcept {
            return t * t * t * t;
        }

        [[nodiscard]] constexpr float EaseOutQuart(float t) noexcept {
            const float f = t - 1.0f;
            return 1.0f - f * f * f * f;
        }

        [[nodiscard]] constexpr float EaseInOutQuart(float t) noexcept {
            return t < 0.5f
                ? 8.0f * t * t * t * t
                : 1.0f - 8.0f * (--t) * t * t * t;
        }

        [[nodiscard]] inline float EaseInExpo(float t) noexcept {
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
        }

        [[nodiscard]] inline float EaseOutExpo(float t) noexcept {
            return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
        }

        [[nodiscard]] inline float EaseInOutExpo(float t) noexcept {
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;

            return t < 0.5f
                ? 0.5f * std::pow(2.0f, 20.0f * t - 10.0f)
                : 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
        }

        [[nodiscard]] inline float EaseInCirc(float t) noexcept {
            return 1.0f - std::sqrt(1.0f - t * t);
        }

        [[nodiscard]] inline float EaseOutCirc(float t) noexcept {
            const float f = t - 1.0f;
            return std::sqrt(1.0f - f * f);
        }

        [[nodiscard]] inline float EaseInOutCirc(float t) noexcept {
            return t < 0.5f
                ? 0.5f * (1.0f - std::sqrt(1.0f - 4.0f * t * t))
                : 0.5f * (std::sqrt(1.0f - 4.0f * (t - 1.0f) * (t - 1.0f)) + 1.0f);
        }

        [[nodiscard]] constexpr float EaseInBack(float t) noexcept {
            constexpr float s = 1.70158f;
            return t * t * ((s + 1.0f) * t - s);
        }

        [[nodiscard]] constexpr float EaseOutBack(float t) noexcept {
            constexpr float s = 1.70158f;
            const float f = t - 1.0f;
            return f * f * ((s + 1.0f) * f + s) + 1.0f;
        }

        [[nodiscard]] constexpr float EaseInOutBack(float t) noexcept {
            constexpr float s = 1.70158f * 1.525f;

            if (t < 0.5f) {
                const float f = 2.0f * t;
                return 0.5f * f * f * ((s + 1.0f) * f - s);
            }

            const float f = 2.0f * t - 2.0f;
            return 0.5f * (f * f * ((s + 1.0f) * f + s) + 2.0f);
        }

        [[nodiscard]] inline float EaseOutElastic(float t) noexcept {
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;

            constexpr float p = 0.3f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t - p / 4.0f) * (2.0f * PI) / p) + 1.0f;
        }

        [[nodiscard]] inline float EaseOutBounce(float t) noexcept {
            constexpr float n1 = 7.5625f;
            constexpr float d1 = 2.75f;

            if (t < 1.0f / d1) {
                return n1 * t * t;
            }
            else if (t < 2.0f / d1) {
                const float f = t - 1.5f / d1;
                return n1 * f * f + 0.75f;
            }
            else if (t < 2.5f / d1) {
                const float f = t - 2.25f / d1;
                return n1 * f * f + 0.9375f;
            }
            else {
                const float f = t - 2.625f / d1;
                return n1 * f * f + 0.984375f;
            }
        }

        // Legacy alias for backward compatibility
        [[nodiscard]] constexpr float EaseInOut(float t) noexcept {
            return EaseInOutQuad(t);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Advanced Smoothing & Damping
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] inline float ExponentialDecay(
            float current,
            float target,
            float decayRate,
            float deltaTime
        ) noexcept {
            return Lerp(current, target, 1.0f - std::exp(-decayRate * deltaTime));
        }

        [[nodiscard]] inline float SmoothDamp(
            float current,
            float target,
            float& currentVelocity,
            float smoothTime,
            float maxSpeed,
            float deltaTime
        ) noexcept {
            smoothTime = std::max(0.0001f, smoothTime);

            const float omega = 2.0f / smoothTime;
            const float x = omega * deltaTime;
            const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

            float change = current - target;
            const float maxChange = maxSpeed * smoothTime;
            change = Clamp(change, -maxChange, maxChange);

            const float temp = (currentVelocity + omega * change) * deltaTime;
            currentVelocity = (currentVelocity - omega * temp) * exp;

            float output = target + (change + temp) * exp;

            if ((target - current > 0.0f) == (output > target)) {
                output = target;
                currentVelocity = 0.0f;
            }

            return output;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometric & Trigonometric Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] constexpr float SmoothStep(
            float edge0,
            float edge1,
            float x
        ) noexcept {
            const float t = Saturate((x - edge0) / (edge1 - edge0));
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] constexpr float DegToRad(float deg) noexcept {
            return deg * (PI / 180.0f);
        }

        [[nodiscard]] constexpr float RadToDeg(float rad) noexcept {
            return rad * (180.0f / PI);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Audio-Specific Transformations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] inline float FreqToMel(float freq) noexcept {
            return 2595.0f * std::log10(1.0f + freq / 700.0f);
        }

        [[nodiscard]] inline float MelToFreq(float mel) noexcept {
            return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
        }

    } // namespace Utils
} // namespace Spectrum

#endif // SPECTRUM_CPP_MATH_UTILS_H