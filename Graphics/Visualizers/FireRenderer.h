#ifndef SPECTRUM_CPP_FIRE_RENDERER_H
#define SPECTRUM_CPP_FIRE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the FireRenderer for pixelated fire effect visualization.
//
// This renderer simulates a fire effect using a 2D grid where heat propagates
// upward with decay. Spectrum data injects heat at the bottom, creating a
// flame-like appearance with color gradients from dark red to white.
//
// Key features:
// - Grid-based fire simulation with heat propagation
// - Optional wind effect (horizontal wave motion)
// - Optional smoothing (neighbor averaging)
// - Quality-dependent pixel size and effects
// - Fixed fire color palette (doesn't support primary color)
//
// Design notes:
// - Grid resolution depends on quality (pixel size)
// - Fire simulation runs in UpdateAnimation()
// - Rendering simply draws grid pixels with palette colors
// - Uses separate read/write buffers for propagation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class FireRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        FireRenderer();
        ~FireRenderer() override = default;

        FireRenderer(const FireRenderer&) = delete;
        FireRenderer& operator=(const FireRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::Fire; }
        [[nodiscard]] std::string_view GetName() const override { return "Fire"; }
        [[nodiscard]] bool SupportsPrimaryColor() const override { return false; }

        void SetPrimaryColor(const Color& /*color*/) override {}
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
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Settings
        {
            bool useSmoothing;
            bool useWind;
            float pixelSize;
            float decay;
            float heatMultiplier;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void InitializeGrid();
        void CreateFirePalette();

        [[nodiscard]] bool CanInitializeGrid() const;
        [[nodiscard]] int CalculateGridWidth() const;
        [[nodiscard]] int CalculateGridHeight() const;
        [[nodiscard]] size_t CalculateGridSize() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Fire Simulation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ApplyDecay();
        void InjectHeat(const SpectrumData& spectrum);
        void PropagateFire();

        void InjectHeatAtPosition(
            int x,
            int bottomY,
            float normalizedValue
        );

        void PropagateCell(
            const std::vector<float>& readGrid,
            int x,
            int y
        );

        [[nodiscard]] float GetCellValue(
            const std::vector<float>& grid,
            int x,
            int y
        ) const;

        [[nodiscard]] int CalculateWindOffset(
            int x,
            float time
        ) const;

        [[nodiscard]] float CalculateSmoothedValue(
            const std::vector<float>& readGrid,
            float centerValue,
            int x,
            int srcY
        ) const;

        [[nodiscard]] int MapSpectrumIndexToGridX(
            size_t spectrumIndex,
            size_t spectrumSize
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderPixel(
            Canvas& canvas,
            int x,
            int y
        ) const;

        void RenderPixelRect(
            Canvas& canvas,
            const Rect& pixelRect,
            const Color& color
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Rect CalculatePixelRect(
            int x,
            int y
        ) const;

        [[nodiscard]] size_t GetGridIndex(
            int x,
            int y
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color GetColorFromPalette(float intensity) const;
        [[nodiscard]] Color ApplyAlphaAdjustment(Color color, float intensity) const;

        [[nodiscard]] Color InterpolatePaletteColors(
            size_t index1,
            size_t index2,
            float t
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsGridValid() const;
        [[nodiscard]] bool IsGridIndexValid(size_t index) const;
        [[nodiscard]] bool IsPixelVisible(float intensity) const;
        [[nodiscard]] bool IsColorVisible(const Color& color) const;
        [[nodiscard]] bool IsBottomRowValid(int bottomY) const;
        [[nodiscard]] bool IsXInBounds(int x) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
        int m_gridWidth;
        int m_gridHeight;
        std::vector<float> m_fireGrid;
        ColorPalette m_firePalette;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_FIRE_RENDERER_H