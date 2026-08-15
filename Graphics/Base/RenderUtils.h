#ifndef SPECTRUM_CPP_RENDER_UTILS_H
#define SPECTRUM_CPP_RENDER_UTILS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// RenderUtils
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"

#include <algorithm>
#include <numeric>
#include <vector>

namespace Spectrum::RenderUtils {

    inline constexpr size_t kBassRatio = 8;
    inline constexpr size_t kMidStartRatio = 8;
    inline constexpr size_t kMidRangeRatio = 2;
    inline constexpr size_t kHighRatio = 8;
    inline constexpr float  kDefaultScale = 0.9f;
    inline constexpr float  kLedHeightScale = 0.95f;
    inline constexpr float  kShadowOffset = 2.0f;
    inline constexpr float  kShadowAlpha = 0.3f;
    inline constexpr float  kLedMinBright = 0.4f;
    inline constexpr float  kLedTopBoost = 1.2f;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Averages
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline float AverageRange(
        const SpectrumData& s, size_t begin, size_t end)
    {
        return std::accumulate(s.begin() + begin, s.begin() + end, 0.0f)
            / float(end - begin);
    }

    [[nodiscard]] inline float SegmentAverage(
        const SpectrumData& s, size_t segments, size_t index)
    {
        const size_t n = s.size();
        return AverageRange(s, index * n / segments, (index + 1) * n / segments);
    }

    [[nodiscard]] inline float GetAverageMagnitude(const SpectrumData& s) {
        return AverageRange(s, 0, s.size());
    }

    [[nodiscard]] inline float GetBassMagnitude(const SpectrumData& s) {
        return AverageRange(s, 0, s.size() / kBassRatio);
    }

    [[nodiscard]] inline float GetMidMagnitude(const SpectrumData& s) {
        const size_t start = s.size() / kMidStartRatio;
        return AverageRange(s, start, start + s.size() / kMidRangeRatio);
    }

    [[nodiscard]] inline float GetHighMagnitude(const SpectrumData& s) {
        return AverageRange(s, s.size() * (kHighRatio - 3) / kHighRatio, s.size());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mapping
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline void BuildPolylineFromSpectrum(
        const SpectrumData& s, float midlineY, float amplitude,
        int viewWidth, std::vector<Point>& out)
    {
        const size_t n = s.size();
        out.resize(n);
        const float step = float(viewWidth) / float(n - 1);
        for (size_t i = 0; i < n; ++i)
            out[i] = { i * step, midlineY - s[i] * amplitude };
    }

    [[nodiscard]] inline float MagnitudeToHeight(
        float magnitude, int viewHeight, float scale = kDefaultScale)
    {
        return magnitude * float(viewHeight) * scale;
    }

    [[nodiscard]] inline int LitRows(float magnitude, int rows, float scale = kLedHeightScale) {
        return int(MagnitudeToHeight(magnitude, 1, scale) * float(rows));
    }

    [[nodiscard]] inline float RowT(int row, int rows) {
        return float(row) / float(rows - 1);
    }

    [[nodiscard]] inline float LedBrightness(float mag, bool top) {
        const float b = Lerp(kLedMinBright, 1.0f, mag);
        return Saturate(top ? b * kLedTopBoost : b);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Shared styles
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline Color ShadowColor(float alpha = kShadowAlpha) {
        return AdjustAlpha(Color::Black(), alpha);
    }

    [[nodiscard]] inline Rect OffsetRect(
        const Rect& r, float dx = kShadowOffset, float dy = kShadowOffset)
    {
        return { r.x + dx, r.y + dy, r.width, r.height };
    }

    [[nodiscard]] inline Color LedIdleColor(bool overlay = false) {
        Color c = AdjustAlpha(Color::FromRGB(80, 80, 80), 0.08f);
        return overlay ? AdjustAlpha(c, c.a * 0.95f) : c;
    }

    [[nodiscard]] inline std::vector<Color> LedGradient() {
        return {
            Color::FromRGB(0, 200, 100), Color::FromRGB(0, 255, 0),
            Color::FromRGB(128, 255, 0), Color::FromRGB(255, 255, 0),
            Color::FromRGB(255, 200, 0), Color::FromRGB(255, 128, 0),
            Color::FromRGB(255, 64, 0),  Color::FromRGB(255, 0, 0),
            Color::FromRGB(200, 0, 50)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Quality
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline int GetMaxBarsForQuality(RenderQuality q) {
        switch (q) {
        case RenderQuality::Low:  return 32;
        case RenderQuality::High: return 128;
        default:                  return 64;
        }
    }

} // namespace Spectrum::RenderUtils

#endif