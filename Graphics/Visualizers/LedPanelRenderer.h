#ifndef SPECTRUM_CPP_LED_PANEL_RENDERER_H
#define SPECTRUM_CPP_LED_PANEL_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class Canvas;

    class LedPanelRenderer final : public BaseRenderer<LedPanelRenderer>
    {
    public:
        LedPanelRenderer();
        ~LedPanelRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::LedPanel;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "LED Panel";
        }

        void OnActivate(int width, int height) override;

    protected:
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
        using Settings = Settings::LedPanelSettings;

        static constexpr float kLedRadius = 6.0f;
        static constexpr float kLedMargin = 3.0f;
        static constexpr float kInactiveAlpha = 0.08f;
        static constexpr float kMinActiveBrightness = 0.4f;
        static constexpr float kHeightScale = 0.95f;
        static constexpr float kTopLedBoost = 1.2f;
        static constexpr float kPeakStrokeWidth = 2.0f;
        static constexpr float kPeakAlpha = 0.8f;
        static constexpr float kColorBlendAmount = 0.7f;

        void UpdateGridConfiguration(size_t requiredColumns);

        void RenderInactiveLeds(Canvas& canvas);

        void RenderActiveLeds(
            Canvas& canvas,
            const SpectrumData& spectrum
        );

        void RenderPeakIndicators(
            Canvas& canvas,
            size_t columnCount
        );

        [[nodiscard]] Point GetLedCenter(
            int col,
            int row
        ) const;

        [[nodiscard]] Color CalculateLedColor(
            float rowNorm,
            float brightness,
            bool isTopLed
        ) const;

        [[nodiscard]] int CalculateActiveLeds(
            float magnitude
        ) const;

        Settings m_settings;
        GridConfig m_grid;
        ColorGradient m_gradient;
    };

} // namespace Spectrum

#endif