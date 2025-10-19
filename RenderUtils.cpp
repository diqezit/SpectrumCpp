// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the rendering utility functions
// These functions extract meaningful values from raw spectrum data and
// help translate that data into drawable screen coordinates
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RenderUtils.h"
#include <numeric>
#include <algorithm>

namespace Spectrum::RenderUtils {

    // prevent out-of-bounds access when calculating average
    float AverageRange(const SpectrumData& spectrum, size_t begin, size_t end) {
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

    // provides an average for a sub-section of the spectrum
    // useful for visualizers that group frequency bands
    float SegmentAverage(const SpectrumData& spectrum, size_t segments, size_t index) {
        if (spectrum.empty() || segments == 0) return 0.0f;

        const size_t start = (index * spectrum.size()) / segments;
        const size_t end = ((index + 1) * spectrum.size()) / segments;
        return AverageRange(spectrum, start, end);
    }

    float GetAverageMagnitude(const SpectrumData& spectrum) {
        return AverageRange(spectrum, 0, spectrum.size());
    }

    // get average of lower frequencies for bass-reactive effects
    float GetBassMagnitude(const SpectrumData& spectrum) {
        if (spectrum.empty()) return 0.0f;
        // use the first 1/8 of spectrum data for bass
        const size_t end = std::max<size_t>(1, spectrum.size() / 8);
        return AverageRange(spectrum, 0, end);
    }

    // get average of middle frequencies
    float GetMidMagnitude(const SpectrumData& spectrum) {
        if (spectrum.empty()) return 0.0f;
        const size_t start = spectrum.size() / 8;
        const size_t end = std::min(spectrum.size(), start + spectrum.size() / 2);
        return AverageRange(spectrum, start, end);
    }

    // get average of high frequencies for treble-reactive effects
    float GetHighMagnitude(const SpectrumData& spectrum) {
        if (spectrum.empty()) return 0.0f;
        const size_t start = std::min(spectrum.size(), (spectrum.size() * 5) / 8);
        return AverageRange(spectrum, start, spectrum.size());
    }

    // calculate width of bars to fit evenly in the view
    BarLayout ComputeBarLayout(size_t count, float spacing, int viewWidth) {
        BarLayout bl{};
        bl.spacing = spacing;
        if (count == 0 || viewWidth <= 0) return bl;

        bl.totalBarWidth = static_cast<float>(viewWidth) / static_cast<float>(count);
        bl.barWidth = bl.totalBarWidth - spacing;
        // prevent negative width if spacing is too large
        if (bl.barWidth < 0.0f) bl.barWidth = 0.0f;
        return bl;
    }

    // generate vertices for a waveform visualizer
    void BuildPolylineFromSpectrum(
        const SpectrumData& spectrum,
        float midlineY,
        float amplitude,
        int viewWidth,
        std::vector<Point>& out
    ) {
        const size_t n = spectrum.size();
        if (n == 0) {
            out.clear();
            return;
        }
        out.resize(n);

        // ensure division by zero is avoided if there's only one point
        const float x_divisor = static_cast<float>(std::max<size_t>(1, n - 1));

        for (size_t i = 0; i < n; ++i) {
            out[i].x = (static_cast<float>(i) / x_divisor) * viewWidth;
            out[i].y = midlineY - spectrum[i] * amplitude;
        }
    }

    // converts a 0-1 magnitude value to a pixel height
    // default scale prevents bars from touching top of the screen
    float MagnitudeToHeight(float magnitude, int viewHeight, float scale) {
        const float h = magnitude * static_cast<float>(viewHeight) * scale;
        // ensure height stays within valid screen bounds
        return std::clamp(h, 0.0f, static_cast<float>(viewHeight));
    }

} // namespace Spectrum::RenderUtils