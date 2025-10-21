// LedPanelRenderer.h
#ifndef SPECTRUM_CPP_LED_PANEL_RENDERER_H
#define SPECTRUM_CPP_LED_PANEL_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the LedPanelRenderer, a grid-based LED matrix visualizer.
//
// This renderer displays spectrum data as a grid of LEDs with vertical
// bars representing frequency bands. LEDs light up progressively based
// on magnitude, with color gradient from green (low) to red (high).
//
// Key features:
// - Dynamic grid sizing based on viewport and spectrum resolution
// - Smooth value transitions with attack/decay rates
// - Peak hold indicators (quality-dependent)
// - Gradient color mapping with optional external color blending
//
// Design notes:
// - All rendering methods are const (state in m_smoothedValues, m_peakValues)
// - Grid recreation on resize/quality change
// - Batch rendering for performance (inactive LEDs in single call)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BaseRenderer.h"

namespace Spectrum {

    class LedPanelRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        LedPanelRenderer();
        ~LedPanelRenderer() override = default;

        LedPanelRenderer(const LedPanelRenderer&) = delete;
        LedPanelRenderer& operator=(const LedPanelRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::LedPanel; }
        [[nodiscard]] std::string_view GetName() const override { return "LED Panel"; }
        [[nodiscard]] bool SupportsPrimaryColor() const override { return true; }

        void OnActivate(int width, int height) override;

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
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings & Data Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings
        {
            bool usePeakHold;
            int maxRows;
            float smoothingMultiplier;
        };

        struct GridData
        {
            int rows = 0;
            int columns = 0;
            float cellSize = 0.0f;
            float startX = 0.0f;
            float startY = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Grid Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateGrid(size_t requiredColumns);
        void CreateGrid(const GridData& gridData);
        void CacheLedPositions();
        void InitializeRowColors();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateValues(const SpectrumData& spectrum);
        void UpdatePeakValues(float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderInactiveLeds(GraphicsContext& context) const;
        void RenderActiveLeds(GraphicsContext& context) const;
        void RenderPeakLeds(GraphicsContext& context) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color GetLedColor(int row, float brightness) const;
        [[nodiscard]] bool HasExternalColor() const;
        [[nodiscard]] Color BlendWithExternalColor(Color baseColor, float t) const;

        [[nodiscard]] static Color InterpolateGradient(float t);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        GridData m_grid;

        std::vector<float> m_smoothedValues;
        std::vector<float> m_peakValues;
        std::vector<float> m_peakTimers;

        std::vector<Point> m_allLedPositions;
        std::vector<Color> m_rowColors;
    };

} // namespace Spectrum

#endif