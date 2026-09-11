#ifndef SPECTRUM_CPP_BARS_RENDERER_H
#define SPECTRUM_CPP_BARS_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BarsRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class BarsRenderer final : public BaseRenderer<BarsRenderer> {
    public:
        BarsRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Bars"; }

    protected:
        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            RectBatch shadows;
            RectBatch bodies;
            std::vector<Bar> highlights;
            highlights.reserve(spectrum.size());

            if (!ForEachBar(
                spectrum, m_settings.barSpacing, kHeightScale, kMinVisibleHeight,
                [&](size_t, float mag, const Rect& rect, const BarLayout&) {
                    const Color color = AdjustBrightness(
                        GetPrimaryColor(), kBrightnessMin + kBrightnessRange * mag);
                    if (m_settings.useShadow)
                        shadows[RenderUtils::ShadowColor()].push_back(RenderUtils::OffsetRect(rect));
                    bodies[color].push_back(rect);
                    highlights.push_back({ rect, mag });
                }))
                return;

            if (m_settings.useShadow)
                RenderRectBatches(ctx, shadows, m_settings.cornerRadius, RoundingMode::Top);
            RenderRectBatches(ctx, bodies, m_settings.cornerRadius, RoundingMode::Top);

            if (!m_settings.useHighlight) return;

            for (const auto& bar : highlights) {
                DrawRoundedRect(
                    ctx,
                    { bar.rect.x, bar.rect.y, bar.rect.width, bar.rect.height * kHighlightRatio },
                    m_settings.cornerRadius,
                    AdjustAlpha(Color::White(), kHighlightAlpha * bar.magnitude),
                    RoundingMode::Top);
            }
        }

    private:
        static constexpr float kHeightScale = 0.9f;
        static constexpr float kMinVisibleHeight = 1.0f;
        static constexpr float kHighlightRatio = 0.15f;
        static constexpr float kHighlightAlpha = 0.25f;
        static constexpr float kBrightnessMin = 0.7f;
        static constexpr float kBrightnessRange = 0.6f;

        struct Bar {
            Rect rect;
            float magnitude = 0.0f;
        };
    };

} // namespace Spectrum

#endif