#ifndef SPECTRUM_CPP_MATRIX_LED_RENDERER_H
#define SPECTRUM_CPP_MATRIX_LED_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the MatrixLedRenderer for grid-based LED matrix visualization.
//
// This renderer displays spectrum data as a grid of square LEDs that light
// up vertically to represent frequency band intensity. Creates a classic
// VU meter LED display effect with smooth transitions and peak indicators.
//
// Key features:
// - Dynamic grid sizing based on viewport and spectrum resolution
// - Smooth attack/decay transitions for realistic LED behavior
// - Peak hold indicators with fade-out (quality-dependent)
// - Gradient color mapping (green to red) with external color blending
// - Batch rendering for optimal performance
//
// Design notes:
// - All rendering methods are const (state in m_smoothedValues, m_peakValues)
// - Grid recalculated on resize/quality changes
// - LED positions pre-calculated and cached
// - Colors pre-computed per row for efficiency
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include <vector>
#include <map>

namespace Spectrum {

    class Canvas;

    class MatrixLedRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        MatrixLedRenderer();
        ~MatrixLedRenderer() override = default;

        MatrixLedRenderer(const MatrixLedRenderer&) = delete;
        MatrixLedRenderer& operator=(const MatrixLedRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::MatrixLed; }
        [[nodiscard]] std::string_view GetName() const override { return "Matrix LED"; }
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
        // Data Structures
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
            bool isOverlay = false;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Grid Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateGridIfNeeded(size_t requiredColumns);
        void RecreateGrid(const GridData& gridData);
        void CacheLedPositions();
        void PrecomputeRowColors();

        [[nodiscard]] GridData CalculateGridLayout(size_t requiredColumns) const;
        [[nodiscard]] bool RequiresGridUpdate(size_t requiredColumns) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSmoothedValues(const SpectrumData& spectrum);
        void UpdatePeakTracking(float deltaTime);
        void UpdateSingleValue(size_t index, float target);
        void UpdateSinglePeak(size_t index, float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Rendering Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderInactiveLeds(Canvas& canvas) const;
        void RenderActiveLeds(Canvas& canvas) const;
        void RenderPeakIndicators(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Batch Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderLedBatches(
            Canvas& canvas,
            const std::map<Color, std::vector<Rect>>& batches
        ) const;

        void RenderPeakBatch(
            Canvas& canvas,
            const std::vector<Rect>& peakRects
        ) const;

        [[nodiscard]] std::map<Color, std::vector<Rect>> GroupActiveLedsByColor() const;
        [[nodiscard]] std::vector<Rect> CollectPeakLeds() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // LED Position Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Rect GetLedRect(int column, int row) const;
        [[nodiscard]] Point GetLedPosition(int column, int row) const;
        [[nodiscard]] int GetActiveLedCount(float value) const;
        [[nodiscard]] int GetPeakLedRow(float peakValue) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color CalculateLedColor(int row, float brightness) const;
        [[nodiscard]] Color GetRowBaseColor(int row) const;
        [[nodiscard]] Color ApplyExternalColorBlend(Color baseColor, float t) const;
        [[nodiscard]] Color GetInactiveLedColor() const;
        [[nodiscard]] Color GetPeakLedColor() const;

        [[nodiscard]] bool HasExternalColor() const;

        [[nodiscard]] static Color InterpolateGradient(float t);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Brightness Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float CalculateLedBrightness(float value) const;
        [[nodiscard]] float CalculateTopLedBrightness(float brightness) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsGridValid() const;
        [[nodiscard]] bool IsColumnValid(int column) const;
        [[nodiscard]] bool IsRowValid(int row) const;
        [[nodiscard]] bool IsPeakVisible(int column) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        GridData m_grid;

        std::vector<float> m_smoothedValues;
        std::vector<float> m_peakValues;
        std::vector<float> m_peakTimers;

        std::vector<Rect> m_allLedRects;
        std::vector<Color> m_rowColors;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_MATRIX_LED_RENDERER_H