#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_VALIDATION_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_VALIDATION_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Validation utilities for geometric and rendering operations
//
// This module provides validation helpers for:
// - Point arrays and geometry data
// - Gradient stops
// - Radius values and ranges
// - Angle values
//
// Design notes:
// - All functions are noexcept for performance
// - Validation is lightweight and inline
// - Returns bool for easy conditional checks
//
// Note: RenderTarget and Brush validation moved to RenderHelpers.h
//       to follow Single Responsibility Principle
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Types.h"
#include <vector>
#include <d2d1.h>
#include <cmath>

namespace Spectrum::Helpers::Validate {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometric Data Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline bool PointArray(const std::vector<Point>& points, size_t minSize = 2) noexcept
    {
        return points.size() >= minSize;
    }

    [[nodiscard]] inline bool GradientStops(const std::vector<D2D1_GRADIENT_STOP>& stops) noexcept
    {
        return stops.size() >= 2;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Radius Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline bool PositiveRadius(float radius) noexcept
    {
        return radius > 0.0f;
    }

    [[nodiscard]] inline bool RadiusRange(float innerRadius, float outerRadius) noexcept
    {
        return innerRadius >= 0.0f && innerRadius < outerRadius;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Angle Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline bool NonZeroAngle(float angle) noexcept
    {
        return std::abs(angle) >= 0.01f;
    }

} // namespace Spectrum::Helpers::Validate

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_VALIDATION_H