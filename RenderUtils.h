#ifndef SPECTRUM_CPP_RENDER_UTILS_H
#define SPECTRUM_CPP_RENDER_UTILS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file provides a collection of stateless utility functions for
// common rendering calculations. These helpers are used by various
// visualizers to analyze spectrum data and compute layout properties
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"

namespace Spectrum::RenderUtils {

    // Spectrum analysis helpers
    float GetAverageMagnitude(const SpectrumData& spectrum);
    float GetBassMagnitude(const SpectrumData& spectrum);
    float GetMidMagnitude(const SpectrumData& spectrum);
    float GetHighMagnitude(const SpectrumData& spectrum);

    // Averaging utilities
    float AverageRange(const SpectrumData& spectrum, size_t begin, size_t end);
    float SegmentAverage(const SpectrumData& spectrum, size_t segments, size_t index);

    // Layout helpers
    struct BarLayout {
        float totalBarWidth = 0.0f;
        float barWidth = 0.0f;
        float spacing = 0.0f;
    };
    BarLayout ComputeBarLayout(size_t count, float spacing, int viewWidth);

    // Geometry helpers
    void BuildPolylineFromSpectrum(
        const SpectrumData& spectrum,
        float midlineY,
        float amplitude,
        int viewWidth,
        std::vector<Point>& out
    );

    float MagnitudeToHeight(float magnitude, int viewHeight, float scale = 0.9f);

} // namespace Spectrum::RenderUtils

#endif