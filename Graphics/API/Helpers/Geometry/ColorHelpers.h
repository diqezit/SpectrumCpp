#ifndef SPECTRUM_CPP_COLOR_HELPERS_H
#define SPECTRUM_CPP_COLOR_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Color Helpers - Color space conversions and manipulation
//
// This module provides:
// - RGB ↔ HSV conversion
// - ARGB uint32 ↔ Color conversion
// - Color interpolation (lerp)
// - Brightness/saturation adjustments
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Types.h"

namespace Spectrum {
    struct Color;
}

namespace Spectrum::Helpers::Color {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // HSV Color Space
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct HSV {
        float h, s, v;

        constexpr HSV(
            float hIn = 0.0f,
            float sIn = 0.0f,
            float vIn = 0.0f
        ) noexcept : h(hIn), s(sIn), v(vIn) {
        }
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Space Conversions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] ::Spectrum::Color HSVtoRGB(const HSV& hsv);
    [[nodiscard]] HSV RGBtoHSV(const ::Spectrum::Color& rgb);

    [[nodiscard]] uint32_t ColorToARGB(const ::Spectrum::Color& color);
    [[nodiscard]] ::Spectrum::Color ARGBtoColor(uint32_t argb);

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Manipulation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] ::Spectrum::Color InterpolateColor(const ::Spectrum::Color& c1, const ::Spectrum::Color& c2, float t);
    [[nodiscard]] ::Spectrum::Color AdjustBrightness(const ::Spectrum::Color& color, float factor);
    [[nodiscard]] ::Spectrum::Color AdjustSaturation(const ::Spectrum::Color& color, float factor);

} // namespace Spectrum::Helpers::Color

#endif // SPECTRUM_CPP_COLOR_HELPERS_H