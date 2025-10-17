#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_VALIDATION_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_VALIDATION_H

#include "Common/Types.h"
#include <vector>
#include <d2d1.h>
#include <cmath>

namespace Spectrum::Helpers::Validate {

    [[nodiscard]] inline bool RenderTarget(ID2D1RenderTarget* rt) noexcept
    {
        return rt != nullptr;
    }

    [[nodiscard]] inline bool RenderTargetAndBrush(ID2D1RenderTarget* rt, ID2D1Brush* brush) noexcept
    {
        return rt != nullptr && brush != nullptr;
    }

    [[nodiscard]] inline bool PointArray(const std::vector<Point>& points, size_t minSize = 2) noexcept
    {
        return points.size() >= minSize;
    }

    [[nodiscard]] inline bool GradientStops(const std::vector<D2D1_GRADIENT_STOP>& stops) noexcept
    {
        return stops.size() >= 2;
    }

    [[nodiscard]] inline bool PositiveRadius(float radius) noexcept
    {
        return radius > 0.0f;
    }

    [[nodiscard]] inline bool RadiusRange(float innerRadius, float outerRadius) noexcept
    {
        return innerRadius >= 0.0f && innerRadius < outerRadius;
    }

    [[nodiscard]] inline bool NonZeroAngle(float angle) noexcept
    {
        return std::abs(angle) >= 0.01f;
    }

} // namespace Spectrum::Helpers::Validate

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_VALIDATION_H