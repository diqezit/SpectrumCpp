// FireRenderer.h: Renders the spectrum as a pixelated fire effect.

#ifndef SPECTRUM_CPP_FIRE_RENDERER_H
#define SPECTRUM_CPP_FIRE_RENDERER_H

#include "BaseRenderer.h"

#include <vector>

namespace Spectrum {

    class FireRenderer final : public BaseRenderer {
    public:
        FireRenderer();
        ~FireRenderer() override = default;

        RenderStyle GetStyle() const override {
            return RenderStyle::Fire;
        }
        std::string_view GetName() const override {
            return "Fire";
        }
        bool SupportsPrimaryColor() const override {
            return false;
        }
        void SetPrimaryColor(const Color& color) override {
            (void)color;
        }

        // re-initialize grid when renderer becomes active or window resizes
        void OnActivate(
            int width,
            int height
        ) override;

    protected:
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
        // settings that change with quality level
        struct Settings {
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
        // Fire Simulation Steps
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ApplyDecay();
        void InjectHeat(const SpectrumData& spectrum);
        void PropagateFire();

        // These helpers are static because they are pure functions
        // and do not depend on the state of a FireRenderer instance.
        static float GetSourcePixelValue(
            const std::vector<float>& readGrid,
            int x,
            int y,
            int gridWidth,
            bool useWind,
            float time
        );

        static float ApplySmoothing(
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
            GraphicsContext& context,
            int x,
            int y
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Color GetColorFromPalette(float intensity) const;

        Settings m_settings;
        int m_gridWidth;
        int m_gridHeight;
        std::vector<float> m_fireGrid;
        ColorPalette m_firePalette;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_FIRE_RENDERER_H