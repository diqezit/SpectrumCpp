#ifndef SPECTRUM_CPP_RENDER_UTILS_H
#define SPECTRUM_CPP_RENDER_UTILS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// RenderUtils — stateless helpers for spectrum analysis and bar layout.
// Header-only.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <vector>
#include <numeric>
#include <algorithm>

namespace Spectrum::RenderUtils {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline constexpr size_t kBassRatio = 8;
    inline constexpr size_t kMidStartRatio = 8;
    inline constexpr size_t kMidRangeRatio = 2;
    inline constexpr size_t kHighRatio = 8;
    inline constexpr float  kDefaultScale = 0.9f;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Range averaging
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] inline float AverageRange(
        const SpectrumData& spectrum,
        size_t begin,
        size_t end)
    {
        if (spectrum.empty()) return 0.0f;

        const size_t n = spectrum.size();
        begin = std::min(begin, n);
        end = std::min(end, n);
        if (begin >= end) return 0.0f;

        const float sum = std::accumulate(
            spectrum.begin() + begin,
            spectrum.begin() + end,
            0.0f);

        return sum / static_cast<float>(end - begin);
    }

    [[nodiscard]] inline float SegmentAverage(
        const SpectrumData& spectrum,
        size_t segments,
        size_t index)
    {
        if (spectrum.empty() || segments == 0 || index >= segments)
            return 0.0f;

        const size_t n = spectrum.size();
        const size_t start = (index * n) / segments;
        const size_t end = ((index + 1) * n) / segments;

        return AverageRange(spectrum, start, end);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Spectrum band analysis
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] inline float GetAverageMagnitude(const SpectrumData& s) {
        return AverageRange(s, 0, s.size());
    }

    [[nodiscard]] inline float GetBassMagnitude(const SpectrumData& s) {
        if (s.empty()) return 0.0f;
        return AverageRange(s, 0, std::max<size_t>(1, s.size() / kBassRatio));
    }

    [[nodiscard]] inline float GetMidMagnitude(const SpectrumData& s) {
        if (s.empty()) return 0.0f;

        const size_t start = s.size() / kMidStartRatio;
        const size_t end = std::min(
            s.size(),
            start + s.size() / kMidRangeRatio);

        return AverageRange(s, start, end);
    }

    [[nodiscard]] inline float GetHighMagnitude(const SpectrumData& s) {
        if (s.empty()) return 0.0f;

        const size_t start = std::min(
            s.size(),
            (s.size() * (kHighRatio - 3)) / kHighRatio);

        return AverageRange(s, start, s.size());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Bar layout
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    struct BarLayout {
        float totalBarWidth = 0.0f;
        float barWidth = 0.0f;
        float spacing = 0.0f;
    };

    [[nodiscard]] inline BarLayout ComputeBarLayout(
        size_t count,
        float spacing,
        int viewWidth)
    {
        BarLayout layout;
        if (count == 0 || viewWidth <= 0)
            return layout;

        layout.spacing = Helpers::Sanitize::NonNegativeFloat(spacing);
        const float width = Helpers::Sanitize::PositiveFloat(
            static_cast<float>(viewWidth), 1.0f);
        layout.totalBarWidth = width / static_cast<float>(count);
        layout.barWidth = std::max(0.0f, layout.totalBarWidth - layout.spacing);

        return layout;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Geometry
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline void BuildPolylineFromSpectrum(
        const SpectrumData& spectrum,
        float midlineY,
        float amplitude,
        int viewWidth,
        std::vector<Point>& out)
    {
        const size_t n = spectrum.size();
        if (n == 0) { out.clear(); return; }

        out.resize(n);

        const float amp = Helpers::Sanitize::NonNegativeFloat(amplitude);
        const float w = Helpers::Sanitize::PositiveFloat(
            static_cast<float>(viewWidth), 1.0f);
        const float div = static_cast<float>(std::max<size_t>(1, n - 1));

        for (size_t i = 0; i < n; ++i) {
            const float mag = Helpers::Sanitize::NormalizedFloat(spectrum[i]);
            out[i].x = (static_cast<float>(i) / div) * w;
            out[i].y = midlineY - mag * amp;
        }
    }

    [[nodiscard]] inline float MagnitudeToHeight(
        float magnitude,
        int viewHeight,
        float scale = kDefaultScale)
    {
        if (viewHeight <= 0) return 0.0f;

        const float mag = Helpers::Sanitize::NormalizedFloat(magnitude);
        const float scl = Helpers::Sanitize::NormalizedFloat(scale);
        const float h = mag * static_cast<float>(viewHeight) * scl;

        return std::clamp(h, 0.0f, static_cast<float>(viewHeight));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Quality helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] inline int GetMaxBarsForQuality(RenderQuality q) {
        switch (q) {
        case RenderQuality::Low:  return 32;
        case RenderQuality::High: return 128;
        default:                  return 64;
        }
    }

} // namespace Spectrum::RenderUtils

#endif