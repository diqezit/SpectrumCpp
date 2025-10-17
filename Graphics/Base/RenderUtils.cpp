// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements rendering utility functions for spectrum analysis and layout.
//
// Implementation details:
// - Frequency band analysis divides spectrum into bass/mid/high ranges
// - Bass: first 1/8 of spectrum (low frequencies)
// - Mid: middle 1/2 of spectrum
// - High: last 3/8 of spectrum (high frequencies)
// - All functions validate input to prevent out-of-bounds access
// - Uses D2DHelpers for sanitization and validation
//
// Performance characteristics:
// - AverageRange: O(n) where n = range size
// - SegmentAverage: O(n) where n = segment size
// - ComputeBarLayout: O(1)
// - BuildPolylineFromSpectrum: O(n) where n = spectrum size
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RenderUtils.h"
#include "Graphics/API/D2DHelpers.h"
#include <numeric>
#include <algorithm>

namespace Spectrum::RenderUtils {

    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Range Utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float AverageRange(
        const SpectrumData& spectrum,
        size_t begin,
        size_t end
    )
    {
        if (spectrum.empty()) return 0.0f;

        const size_t n = spectrum.size();
        begin = std::min(begin, n);
        end = std::min(end, n);

        if (begin >= end) return 0.0f;

        const float sum = std::accumulate(
            spectrum.begin() + begin,
            spectrum.begin() + end,
            0.0f
        );

        return sum / static_cast<float>(end - begin);
    }

    float SegmentAverage(
        const SpectrumData& spectrum,
        size_t segments,
        size_t index
    )
    {
        if (spectrum.empty() || segments == 0) return 0.0f;
        if (index >= segments) return 0.0f;

        const size_t n = spectrum.size();
        const size_t start = (index * n) / segments;
        const size_t end = ((index + 1) * n) / segments;

        return AverageRange(spectrum, start, end);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Analysis
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float GetAverageMagnitude(const SpectrumData& spectrum)
    {
        return AverageRange(spectrum, 0, spectrum.size());
    }

    float GetBassMagnitude(const SpectrumData& spectrum)
    {
        if (spectrum.empty()) return 0.0f;

        const size_t end = std::max<size_t>(1, spectrum.size() / kBassFrequencyRatio);
        return AverageRange(spectrum, 0, end);
    }

    float GetMidMagnitude(const SpectrumData& spectrum)
    {
        if (spectrum.empty()) return 0.0f;

        const size_t start = spectrum.size() / kMidFrequencyStartRatio;
        const size_t end = std::min(
            spectrum.size(),
            start + spectrum.size() / kMidFrequencyRangeRatio
        );

        return AverageRange(spectrum, start, end);
    }

    float GetHighMagnitude(const SpectrumData& spectrum)
    {
        if (spectrum.empty()) return 0.0f;

        const size_t start = std::min(
            spectrum.size(),
            (spectrum.size() * (kHighFrequencyRatio - 3)) / kHighFrequencyRatio
        );

        return AverageRange(spectrum, start, spectrum.size());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Layout Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    BarLayout ComputeBarLayout(
        size_t count,
        float spacing,
        int viewWidth
    )
    {
        BarLayout layout{};

        if (count == 0 || viewWidth <= 0) return layout;

        const float sanitizedSpacing = NonNegativeFloat(spacing);
        const float sanitizedWidth = PositiveFloat(
            static_cast<float>(viewWidth),
            1.0f
        );

        layout.spacing = sanitizedSpacing;
        layout.totalBarWidth = sanitizedWidth / static_cast<float>(count);
        layout.barWidth = std::max(0.0f, layout.totalBarWidth - sanitizedSpacing);

        return layout;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BuildPolylineFromSpectrum(
        const SpectrumData& spectrum,
        float midlineY,
        float amplitude,
        int viewWidth,
        std::vector<Point>& out
    )
    {
        const size_t n = spectrum.size();

        if (n == 0) {
            out.clear();
            return;
        }

        out.resize(n);

        const float sanitizedAmplitude = NonNegativeFloat(amplitude);
        const float sanitizedWidth = PositiveFloat(
            static_cast<float>(viewWidth),
            1.0f
        );
        const float xDivisor = static_cast<float>(std::max<size_t>(1, n - 1));

        for (size_t i = 0; i < n; ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);

            out[i].x = (static_cast<float>(i) / xDivisor) * sanitizedWidth;
            out[i].y = midlineY - magnitude * sanitizedAmplitude;
        }
    }

    float MagnitudeToHeight(
        float magnitude,
        int viewHeight,
        float scale
    )
    {
        if (viewHeight <= 0) return 0.0f;

        const float sanitizedMagnitude = NormalizedFloat(magnitude);
        const float sanitizedScale = NormalizedFloat(scale);
        const float height = sanitizedMagnitude * static_cast<float>(viewHeight) * sanitizedScale;

        return std::clamp(height, 0.0f, static_cast<float>(viewHeight));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Quality-based Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    int GetMaxBarsForQuality(RenderQuality quality)
    {
        switch (quality)
        {
        case RenderQuality::Low:
            return 32;
        case RenderQuality::High:
            return 128;
        case RenderQuality::Medium:
        default:
            return 64;
        }
    }

} // namespace Spectrum::RenderUtils