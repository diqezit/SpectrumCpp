#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_SANITIZATION_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_SANITIZATION_H

#include <algorithm>

namespace Spectrum::Helpers::Sanitize {

    [[nodiscard]] inline float PositiveFloat(float value, float defaultValue = 0.0f) noexcept
    {
        return (value > 0.0f) ? value : defaultValue;
    }

    [[nodiscard]] inline float NonNegativeFloat(float value) noexcept
    {
        return std::max(value, 0.0f);
    }

    [[nodiscard]] inline float NormalizedFloat(float value) noexcept
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    [[nodiscard]] inline int MinValue(int value, int minValue) noexcept
    {
        return std::max(value, minValue);
    }

    [[nodiscard]] inline int ClampValue(int value, int minValue, int maxValue) noexcept
    {
        return std::clamp(value, minValue, maxValue);
    }

    [[nodiscard]] inline float Radius(float value) noexcept
    {
        return PositiveFloat(value, 1.0f);
    }

    [[nodiscard]] inline int PolygonSides(int sides) noexcept
    {
        return std::max(sides, 3);
    }

    [[nodiscard]] inline int StarPoints(int points) noexcept
    {
        return std::max(points, 2);
    }

    [[nodiscard]] inline int CircleSegments(int segments) noexcept
    {
        return std::clamp(segments, 3, 360);
    }

} // namespace Spectrum::Helpers::Sanitize

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_SANITIZATION_H