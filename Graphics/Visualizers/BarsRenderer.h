#ifndef SPECTRUM_CPP_BARS_RENDERER_H
#define SPECTRUM_CPP_BARS_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class BarsRenderer final : public BaseRenderer<BarsRenderer>
    {
    public:
        BarsRenderer();
        ~BarsRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Bars;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Bars";
        }

    protected:
        void UpdateSettings() override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        using Settings = Settings::BarsSettings;

        static constexpr float kHeightScale = 0.9f;
        static constexpr float kMinVisibleHeight = 1.0f;
        static constexpr float kHighlightHeightRatio = 0.15f;
        static constexpr float kHighlightAlpha = 0.25f;
        static constexpr float kBrightnessMin = 0.7f;
        static constexpr float kBrightnessRange = 0.6f;
        static constexpr float kShadowOffsetX = 2.0f;
        static constexpr float kShadowOffsetY = 2.0f;
        static constexpr float kShadowAlpha = 0.3f;

        struct BarData {
            Rect rect;
            float magnitude;
            Color color;
        };

        [[nodiscard]] std::vector<BarData> CollectVisibleBars(
            const SpectrumData& spectrum,
            const BarLayout& layout
        ) const;

        void RenderBarShadows(
            Canvas& canvas,
            const std::vector<BarData>& bars
        ) const;

        void RenderBarBodies(
            Canvas& canvas,
            const std::vector<BarData>& bars
        ) const;

        void RenderBarHighlights(
            Canvas& canvas,
            const std::vector<BarData>& bars
        ) const;

        void DrawRoundedTopBar(
            Canvas& canvas,
            const Rect& rect,
            float radius,
            const Paint& paint
        ) const;

        [[nodiscard]] Color CalculateBarColor(
            float magnitude
        ) const;

        [[nodiscard]] Rect CalculateHighlightRect(
            const Rect& barRect
        ) const;

        Settings m_settings;
    };

} // namespace Spectrum

#endif