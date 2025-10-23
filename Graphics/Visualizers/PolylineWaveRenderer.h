#ifndef SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H
#define SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PolylineWaveRenderer for enhanced sunburst visualization
//
// Dynamic sunburst pattern with advanced visual effects:
// - Gradient bars with color transitions from center to edge
// - Pulsating inner core responding to audio intensity
// - Multi-layer rendering with depth and glow effects
// - Dynamic highlights on intensity peaks
// - Smooth color interpolation for visual appeal
// - Uses GeometryHelpers for all geometric calculations
//
// Performance optimized with pre-calculated directions and minimal allocations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>

namespace Spectrum {

    class Canvas;

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

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        using Settings = Settings::PolylineWaveSettings;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void EnsureBarDirections(size_t barCount);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateCoreRadius(const SpectrumData& spectrum, float deltaTime);
        [[nodiscard]] float CalculateAverageIntensity(const SpectrumData& spectrum) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderOuterGlow(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float baseRadius,
            float barWidth
        ) const;

        void RenderPulsingCore(
            Canvas& canvas,
            const Point& center,
            float baseRadius
        ) const;

        void RenderGradientBars(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float baseRadius,
            float barWidth
        ) const;

        void RenderSolidBars(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float baseRadius,
            float barWidth
        ) const;

        void RenderDynamicHighlights(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float baseRadius,
            float barWidth
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Individual Bar Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderGradientBar(
            Canvas& canvas,
            const Point& center,
            const Point& direction,
            float baseRadius,
            float barLength,
            float barWidth,
            float magnitude
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float CalculateBaseRadius() const;

        [[nodiscard]] float CalculateBarWidth(
            size_t barCount,
            float radius
        ) const;

        [[nodiscard]] float CalculateBarLength(
            float magnitude,
            float radius
        ) const;

        [[nodiscard]] Color CalculateBarColorAtPosition(
            float normalizedPosition,
            float magnitude
        ) const;

        [[nodiscard]] Color GetAccentColor() const;
        [[nodiscard]] Point GetViewportCenter() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ShouldRenderBar(float magnitude) const;
        [[nodiscard]] bool ShouldRenderGlow(float magnitude) const;
        [[nodiscard]] bool ShouldRenderHighlight(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
        std::vector<Point> m_barDirections;
        float m_currentCoreRadius;
        float m_targetCoreRadius;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H