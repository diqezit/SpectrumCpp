#ifndef SPECTRUM_CPP_COLOR_WHEEL_GENERATOR_H
#define SPECTRUM_CPP_COLOR_WHEEL_GENERATOR_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// HSV color wheel bitmap generator
//
// Single-pass generation with inlined HSV conversion
// Transparent pixels outside radius for circular appearance
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Helpers/Geometry/ColorHelpers.h"
#include <vector>
#include <cmath>

namespace Spectrum {

    class ColorWheelGenerator {
    public:
        static std::vector<uint32_t> GenerateBitmapData(int size, float radius)
        {
            std::vector<uint32_t> pixels(static_cast<size_t>(size) * size, 0u);

            for (int y = 0; y < size; ++y)
            {
                const float fy = static_cast<float>(y) - radius;
                for (int x = 0; x < size; ++x)
                {
                    const float fx = static_cast<float>(x) - radius;
                    const float dist = std::sqrt(fx * fx + fy * fy);

                    if (dist > radius)
                    {
                        pixels[static_cast<size_t>(y) * size + x] = 0u;
                    }
                    else
                    {
                        const float hue = (std::atan2(fy, fx) / PI + 1.0f) * 0.5f;
                        const float sat = dist / radius;
                        const Color rgb = Helpers::Color::HSVtoRGB({ hue, sat, 1.0f });
                        pixels[static_cast<size_t>(y) * size + x] = Helpers::Color::ColorToARGB(rgb);
                    }
                }
            }

            return pixels;
        }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_COLOR_WHEEL_GENERATOR_H