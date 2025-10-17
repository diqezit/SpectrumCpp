#ifndef SPECTRUM_CPP_RENDER_UTILS_H
#define SPECTRUM_CPP_RENDER_UTILS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines a collection of stateless utility functions for common rendering
// calculations and spectrum data analysis.
//
// This namespace provides pure functions (no side effects) for:
// - Spectrum frequency band analysis (bass, mid, high, average)
// - Range averaging and segmentation
// - Bar layout calculations for visualizers
// - Waveform point generation
// - Magnitude to screen coordinate conversion
//
// Design notes:
// - All functions are stateless (no internal state)
// - All functions are [[nodiscard]] (return values must be used)
// - All functions validate input parameters
// - Uses D2DHelpers for sanitization and validation
//
// Usage pattern:
//   float bass = RenderUtils::GetBassMagnitude(spectrum);
//   auto layout = RenderUtils::ComputeBarLayout(count, spacing, width);
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <vector>

namespace Spectrum::RenderUtils {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline constexpr size_t kBassFrequencyRatio = 8;
    inline constexpr size_t kMidFrequencyStartRatio = 8;
    inline constexpr size_t kMidFrequencyRangeRatio = 2;
    inline constexpr size_t kHighFrequencyRatio = 8;
    inline constexpr float kDefaultHeightScale = 0.9f;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Analysis
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float GetAverageMagnitude(const SpectrumData& spectrum);
    [[nodiscard]] float GetBassMagnitude(const SpectrumData& spectrum);
    [[nodiscard]] float GetMidMagnitude(const SpectrumData& spectrum);
    [[nodiscard]] float GetHighMagnitude(const SpectrumData& spectrum);

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Range Utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float AverageRange(
        const SpectrumData& spectrum,
        size_t begin,
        size_t end
    );

    [[nodiscard]] float SegmentAverage(
        const SpectrumData& spectrum,
        size_t segments,
        size_t index
    );

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Layout Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct BarLayout
    {
        float totalBarWidth = 0.0f;
        float barWidth = 0.0f;
        float spacing = 0.0f;
    };

    [[nodiscard]] BarLayout ComputeBarLayout(
        size_t count,
        float spacing,
        int viewWidth
    );

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BuildPolylineFromSpectrum(
        const SpectrumData& spectrum,
        float midlineY,
        float amplitude,
        int viewWidth,
        std::vector<Point>& out
    );

    [[nodiscard]] float MagnitudeToHeight(
        float magnitude,
        int viewHeight,
        float scale = kDefaultHeightScale
    );

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Quality-based Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] int GetMaxBarsForQuality(RenderQuality quality);

} // namespace Spectrum::RenderUtils

#endif // SPECTRUM_CPP_RENDER_UTILS_H