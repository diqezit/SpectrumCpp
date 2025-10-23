#ifndef SPECTRUM_CPP_COLOR_UTILS_H
#define SPECTRUM_CPP_COLOR_UTILS_H

#include "Common/Common.h"

namespace Spectrum {
    namespace Utils {

        struct HSV {
            float h, s, v;
            HSV(
                float hIn = 0.0f,
                float sIn = 0.0f,
                float vIn = 0.0f
            ) : h(hIn), s(sIn), v(vIn) {
            }
        };

        Color HSVtoRGB(const HSV& hsv);
        HSV   RGBtoHSV(const Color& rgb);

        uint32_t ColorToARGB(const Color& color);
        Color    ARGBtoColor(uint32_t argb);

        Color InterpolateColor(const Color& c1, const Color& c2, float t);
        Color AdjustBrightness(const Color& color, float factor);
        Color AdjustSaturation(const Color& color, float factor);

    } // namespace Utils
} // namespace Spectrum

#endif