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

#include "BaseRenderer.h"
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

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Fire Simulation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ApplyDecay();
        void InjectHeat(const SpectrumData& spectrum);
        void PropagateFire();

        [[nodiscard]] static float GetSourcePixelValue(
            const std::vector<float>& readGrid,
            int x,
            int y,
            int gridWidth,
            bool useWind,
            float time
        );

        [[nodiscard]] static float ApplySmoothing(
            const std::vector<float>& readGrid,
            float currentValue,
            int x,
            int y,
            int gridWidth,
            bool useSmoothing
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderPixel(
            Canvas& canvas,
            int x,
            int y
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color GetColorFromPalette(float intensity) const;

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