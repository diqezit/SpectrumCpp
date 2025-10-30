#ifndef SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H
#define SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class Canvas;

    class KenwoodBarsRenderer final : public BaseRenderer<KenwoodBarsRenderer>
    {
    public:
        KenwoodBarsRenderer();
        ~KenwoodBarsRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::KenwoodBars;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Kenwood Bars";
        }

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(Canvas& canvas, const SpectrumData& spectrum) override;

    private:
        using Settings = Settings::KenwoodBarsSettings;

        [[nodiscard]] BarStyle CreateBarStyle() const;

        Settings m_settings;
    };

} // namespace Spectrum

#endif