#ifndef SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H
#define SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PolylineWaveRenderer for sunburst/starburst visualization.
//
// This renderer displays spectrum data as radial bars emanating from a
// central point, creating a dynamic sunburst pattern that pulses with
// the audio intensity.
//
// Key features:
// - Bars radiate outward from center in circular arrangement
// - Bar length modulated by frequency magnitude
// - Multi-layer rendering with glow and highlight effects
// - Dynamic bar width calculation based on count
// - Pre-calculated directional vectors for performance
//
// Design notes:
// - All rendering methods are const (state in m_barDirections)
// - Quality settings control visual effects and performance
// - Layered rendering ensures proper alpha blending
// - Bar directions cached to avoid repeated trigonometry
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include <vector>

namespace Spectrum {

    class Canvas;
    class Paint;  // Forward declaration added

    class PolylineWaveRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        PolylineWaveRenderer();
        ~PolylineWaveRenderer() override = default;

        PolylineWaveRenderer(const PolylineWaveRenderer&) = delete;
        PolylineWaveRenderer& operator=(const PolylineWaveRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::PolylineWave; }
        [[nodiscard]] std::string_view GetName() const override { return "Sunburst"; }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BaseRenderer Overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSettings() override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Data Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings
        {
            bool useGlow;
            bool useHighlight;
            float glowIntensity;
            float innerCircleAlpha;
            float barSpacingRatio;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Rendering Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderInnerCircle(
            Canvas& canvas,
            const Point& center,
            float radius
        ) const;

        void RenderGlowLayer(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float radius,
            float barWidth
        ) const;

        void RenderMainBars(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float radius,
            float barWidth
        ) const;

        void RenderHighlightLayer(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float radius,
            float barWidth
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void EnsureBarDirections(size_t barCount);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float CalculateRadius() const;

        [[nodiscard]] float CalculateBarWidth(
            size_t barCount,
            float radius
        ) const;

        [[nodiscard]] float CalculateBarLength(
            float magnitude,
            float radius
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        std::vector<Point> m_barDirections;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H