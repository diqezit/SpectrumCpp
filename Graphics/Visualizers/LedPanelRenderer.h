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

#include "Graphics/Base/BaseRenderer.h"
#include <vector>

namespace Spectrum {

    class Canvas;

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
            Canvas& canvas,
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

        [[nodiscard]] GridData CalculateGridData(size_t requiredColumns) const;
        [[nodiscard]] int CalculateGridColumns(size_t requiredColumns, float availableWidth, float ledSize) const;
        [[nodiscard]] int CalculateGridRows(float availableHeight, float ledSize) const;
        [[nodiscard]] float CalculateCellSize(int cols, int rows, float availableWidth, float availableHeight) const;
        [[nodiscard]] float GetAvailableWidth() const;
        [[nodiscard]] float GetAvailableHeight() const;
        [[nodiscard]] float GetLedSize() const;

        [[nodiscard]] Point CalculateLedPosition(int col, int row) const;
        [[nodiscard]] size_t CalculateTotalLedCount() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateValues(const SpectrumData& spectrum);
        void UpdateColumnValue(size_t index, float targetValue);

        void UpdatePeakValues(float deltaTime);
        void UpdatePeak(size_t index, float currentValue, float deltaTime);
        void UpdatePeakHoldTimer(size_t index, float deltaTime);
        void UpdatePeakDecay(size_t index);

        [[nodiscard]] float CalculateSmoothedValue(float current, float target) const;
        [[nodiscard]] float GetSmoothingRate(float current, float target) const;
        [[nodiscard]] size_t GetUpdateCount(const SpectrumData& spectrum) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderInactiveLeds(Canvas& canvas) const;
        void RenderActiveLeds(Canvas& canvas) const;
        void RenderPeakLeds(Canvas& canvas) const;

        void RenderColumnLeds(Canvas& canvas, int col) const;
        void RenderSingleLed(Canvas& canvas, size_t ledIndex, const Color& color) const;
        void RenderPeakLed(Canvas& canvas, int col, int peakRow) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] int CalculateActiveLedCount(float value) const;
        [[nodiscard]] int CalculatePeakRow(float peakValue) const;
        [[nodiscard]] size_t GetLedIndex(int col, int row) const;
        [[nodiscard]] float GetGridStartX(float gridWidth) const;
        [[nodiscard]] float GetGridStartY(float gridHeight) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color GetLedColor(int row, float brightness) const;
        [[nodiscard]] Color GetRowBaseColor(int row) const;
        [[nodiscard]] Color ApplyBrightness(Color color, float brightness) const;
        [[nodiscard]] Color GetInactiveColor() const;
        [[nodiscard]] Color GetPeakColor() const;

        [[nodiscard]] float CalculateBrightness(float value) const;
        [[nodiscard]] float CalculateLedBrightness(float baseBrightness, bool isTopLed) const;

        [[nodiscard]] bool HasExternalColor() const;
        [[nodiscard]] Color BlendWithExternalColor(Color baseColor, float t) const;
        [[nodiscard]] float GetColorBlendRatio(int row) const;

        [[nodiscard]] static Color InterpolateGradient(float t);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsGridValid() const;
        [[nodiscard]] bool CanUpdateGrid(size_t requiredColumns) const;
        [[nodiscard]] bool ShouldRecreateGrid(size_t requiredColumns) const;
        [[nodiscard]] bool IsValidViewportSize() const;
        [[nodiscard]] bool IsColumnIndexValid(size_t index) const;
        [[nodiscard]] bool IsRowIndexValid(int row) const;
        [[nodiscard]] bool IsLedIndexValid(size_t index) const;
        [[nodiscard]] bool ShouldRenderMinimumLed(float value, int activeLeds) const;
        [[nodiscard]] bool IsPeakVisible(size_t index) const;
        [[nodiscard]] bool IsPeakRowValid(int row) const;

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

#endif // SPECTRUM_CPP_LED_PANEL_RENDERER_H