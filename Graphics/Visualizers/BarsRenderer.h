#ifndef SPECTRUM_CPP_BARS_RENDERER_H
#define SPECTRUM_CPP_BARS_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BarsRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class BarsRenderer final : public BaseRenderer<BarsRenderer> {
    public:
        BarsRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Bars"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::BarsSettings>();
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            const auto layout = CalculateBarLayout(spectrum.size(), m_settings.barSpacing);
            if (layout.barWidth <= 0.0f) return;

            RectBatch shadows;
            RectBatch bodies;
            std::vector<Bar> highlights;
            highlights.reserve(spectrum.size());

            for (size_t i = 0; i < spectrum.size(); ++i) {
                const float mag = Helpers::Sanitize::Normalized(spectrum[i]);
                const float height = RenderUtils::MagnitudeToHeight(mag, GetHeight(), kHeightScale);
                if (height < kMinVisibleHeight) continue;

                const Rect rect = GetBarRect(layout, i, height);
                const Color color = AdjustBrightness(
                    GetPrimaryColor(), kBrightnessMin + kBrightnessRange * mag);

                if (m_settings.useShadow)
                    shadows[RenderUtils::ShadowColor()].push_back(RenderUtils::OffsetRect(rect));
                bodies[color].push_back(rect);
                highlights.push_back({ rect, mag });
            }

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

        Settings::BarsSettings m_settings{};
    };

} // namespace Spectrum

#endif