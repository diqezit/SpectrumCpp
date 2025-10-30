#ifndef SPECTRUM_CPP_FIRE_RENDERER_H
#define SPECTRUM_CPP_FIRE_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class FireRenderer final : public BaseRenderer<FireRenderer>
    {
    public:
        FireRenderer();
        ~FireRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Fire;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Fire";
        }

        [[nodiscard]] bool SupportsPrimaryColor() const override {
            return false;
        }

        void SetPrimaryColor(const Color&) override {}
        void OnActivate(int width, int height) override;

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(Canvas& canvas, const SpectrumData& spectrum) override;

    private:
        using Settings = Settings::FireSettings;

        void InitializeGrid();
        void CreateFirePalette();
        void InjectHeat(const SpectrumData& spectrum);
        void PropagateFire();

        Settings m_settings;
        int m_gridWidth;
        int m_gridHeight;
        std::vector<float> m_fireGrid;
        ColorGradient m_firePalette;
    };

} // namespace Spectrum

#endif