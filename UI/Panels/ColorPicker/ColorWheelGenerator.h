#ifndef SPECTRUM_CPP_COLOR_WHEEL_GENERATOR_H
#define SPECTRUM_CPP_COLOR_WHEEL_GENERATOR_H

#include "Common/Common.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include <vector>
#include <cmath>

namespace Spectrum {

    class ColorWheelGenerator {
    public:
        static std::vector<uint32_t> GenerateBitmapData(
            int size,
            float radius
        ) {
            std::vector<uint32_t> pixels(static_cast<size_t>(size) * size, 0u);

            for (int y = 0; y < size; ++y) {
                const float fy = static_cast<float>(y) - radius;
                for (int x = 0; x < size; ++x) {
                    const float fx = static_cast<float>(x) - radius;
                    pixels[static_cast<size_t>(y) * size + x] = MakeWheelPixel(fx, fy, radius);
                }
            }
            return pixels;
        }

    private:
        static uint32_t MakeWheelPixel(
            float dx,
            float dy,
            float radius
        ) {
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) {
                return 0u; // Transparent pixel outside the wheel
            }

            const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
            const float sat = dist / radius;
            const Color rgb = Utils::HSVtoRGB({ hue, sat, 1.0f });

            return Utils::ColorToARGB(rgb);
        }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_COLOR_WHEEL_GENERATOR_H