// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FireRenderer.h: Renders the spectrum as a pixelated fire effect.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_FIRE_RENDERER_H
#define SPECTRUM_CPP_FIRE_RENDERER_H

#include "BaseRenderer.h"

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
        void OnActivate(int width, int height) override;

    protected:
        void UpdateSettings() override;
        void UpdateAnimation(const SpectrumData& spectrum,
            float deltaTime) override;
        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        void InitializeGrid();
        void CreateFirePalette();
        Color GetColorFromPalette(float intensity) const;

        struct Settings {
            bool useSmoothing;
            bool useWind;
            float pixelSize;
            float decay;
            float heatMultiplier;
        };

        Settings m_settings;
        int m_gridWidth;
        int m_gridHeight;
        std::vector<float> m_fireGrid;
        ColorPalette m_firePalette;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_FIRE_RENDERER_H