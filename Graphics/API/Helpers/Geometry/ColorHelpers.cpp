#include "ColorHelpers.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include <algorithm>
#include <cmath>
#include <array>

namespace Spectrum::Helpers::Color {

    using namespace Helpers::Math;
    using ::Spectrum::Color;

    namespace {
        constexpr float kColorDeltaEpsilon = 1e-6f;
        constexpr float kOneSixth = 1.0f / 6.0f;
        constexpr float kOneThird = 1.0f / 3.0f;
        constexpr float kTwoThirds = 2.0f / 3.0f;
    }

    Color HSVtoRGB(const HSV& hsv) {
        float s = Saturate(hsv.s);
        float v = Saturate(hsv.v);

        if (s <= 0.0f) {
            return Color(v, v, v, 1.0f);
        }

        float h = hsv.h;
        if (h < 0.0f || h >= 1.0f) {
            h = h - std::floor(h);
        }
        h *= 6.0f;

        const int sector = static_cast<int>(h);
        const float fraction = h - static_cast<float>(sector);

        const float p = v * (1.0f - s);
        const float q = v * (1.0f - s * fraction);
        const float t = v * (1.0f - s * (1.0f - fraction));

        const int sectorMod = sector % 6;

        constexpr std::array<std::array<int, 3>, 6> colorTable = { {
            {0, 2, 1},
            {3, 0, 1},
            {1, 0, 2},
            {1, 3, 0},
            {2, 1, 0},
            {0, 1, 3}
        } };

        const std::array<float, 4> values = { v, p, t, q };
        const auto& indices = colorTable[sectorMod];

        return Color(
            values[indices[0]],
            values[indices[1]],
            values[indices[2]],
            1.0f
        );
    }

    HSV RGBtoHSV(const Color& rgb) {
        const float r = Saturate(rgb.r);
        const float g = Saturate(rgb.g);
        const float b = Saturate(rgb.b);

        const float maxV = std::max({ r, g, b });
        const float minV = std::min({ r, g, b });
        const float delta = maxV - minV;

        HSV out{};
        out.v = maxV;

        if (delta < kColorDeltaEpsilon) {
            out.s = 0.0f;
            out.h = 0.0f;
            return out;
        }

        out.s = (maxV <= 0.0f) ? 0.0f : (delta / maxV);

        if (r >= maxV) {
            out.h = (g - b) / delta;
        }
        else if (g >= maxV) {
            out.h = 2.0f + (b - r) / delta;
        }
        else {
            out.h = 4.0f + (r - g) / delta;
        }

        out.h /= 6.0f;
        if (out.h < 0.0f) {
            out.h += 1.0f;
        }

        return out;
    }

    uint32_t ColorToARGB(const Color& color) {
        const auto toByte = [](float c) -> uint8_t {
            return static_cast<uint8_t>(
                Clamp(std::round(c * 255.0f), 0.0f, 255.0f)
                );
            };

        const uint8_t a = toByte(color.a);
        const uint8_t r = toByte(color.r);
        const uint8_t g = toByte(color.g);
        const uint8_t b = toByte(color.b);

        return (static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(r) << 16) |
            (static_cast<uint32_t>(g) << 8) |
            static_cast<uint32_t>(b);
    }

    Color ARGBtoColor(uint32_t argb) {
        const float a = static_cast<float>((argb >> 24) & 0xFF) / 255.0f;
        const float r = static_cast<float>((argb >> 16) & 0xFF) / 255.0f;
        const float g = static_cast<float>((argb >> 8) & 0xFF) / 255.0f;
        const float b = static_cast<float>((argb) & 0xFF) / 255.0f;
        return Color(r, g, b, a);
    }

    Color InterpolateColor(const Color& c1, const Color& c2, float t) {
        const float ct = Saturate(t);
        const float oneMinusT = 1.0f - ct;

        return Color(
            c1.r * oneMinusT + c2.r * ct,
            c1.g * oneMinusT + c2.g * ct,
            c1.b * oneMinusT + c2.b * ct,
            c1.a * oneMinusT + c2.a * ct
        );
    }

    Color AdjustBrightness(const Color& color, float factor) {
        return Color(
            Clamp(color.r * factor, 0.0f, 1.0f),
            Clamp(color.g * factor, 0.0f, 1.0f),
            Clamp(color.b * factor, 0.0f, 1.0f),
            color.a
        );
    }

    Color AdjustSaturation(const Color& color, float factor) {
        HSV hsv = RGBtoHSV(color);
        hsv.s = Clamp(hsv.s * factor, 0.0f, 1.0f);
        Color out = HSVtoRGB(hsv);
        out.a = color.a;
        return out;
    }

} // namespace Spectrum::Helpers::Color