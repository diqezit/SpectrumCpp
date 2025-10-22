#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_MATH_CONSTANTS_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_MATH_CONSTANTS_H

#include <cmath>

// C++20 numbers library with fallback
#if __cpp_lib_math_constants >= 201907L
#include <numbers>
#endif

namespace Spectrum::Helpers::Math {

    // Mathematical constants
#if __cpp_lib_math_constants >= 201907L
    inline constexpr float kPi = std::numbers::pi_v<float>;
#else
    inline constexpr float kPi = 3.14159265358979323846f;
#endif

    inline constexpr float kTwoPi = kPi * 2.0f;
    inline constexpr float kHalfPi = kPi * 0.5f;
    inline constexpr float kDegToRad = kPi / 180.0f;
    inline constexpr float kRadToDeg = 180.0f / kPi;

    // Angle conversion utilities
    [[nodiscard]] inline float DegreesToRadians(float degrees) noexcept
    {
        return degrees * kDegToRad;
    }

    [[nodiscard]] inline float RadiansToDegrees(float radians) noexcept
    {
        return radians * kRadToDeg;
    }

} // namespace Spectrum::Helpers::Math

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_MATH_CONSTANTS_H