#include "Graphics/Visualizers/BarsRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Math;

    BarsRenderer::BarsRenderer() {
        UpdateSettings();
    }

    void BarsRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
    }

    void BarsRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        const auto layout = CalculateBarLayout(
            spectrum.size(),
            m_settings.barSpacing
        );

        if (layout.barWidth <= 0.0f) return;

        const auto bars = CollectVisibleBars(spectrum, layout);
        if (bars.empty()) return;

        if (m_settings.useShadow) {
            RenderBarShadows(canvas, bars);
        }

        RenderBarBodies(canvas, bars);

        if (m_settings.useHighlight) {
            RenderBarHighlights(canvas, bars);
        }
    }

    std::vector<BarsRenderer::BarData> BarsRenderer::CollectVisibleBars(
        const SpectrumData& spectrum,
        const BarLayout& layout
    ) const {
        std::vector<BarData> bars;
        bars.reserve(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);
            const float height = RenderUtils::MagnitudeToHeight(
                magnitude,
                GetHeight(),
                kHeightScale
            );

            if (height < kMinVisibleHeight) continue;

            BarData bar;
            bar.rect = GetBarRect(layout, i, height);
            bar.magnitude = magnitude;
            bar.color = CalculateBarColor(magnitude);

            bars.push_back(bar);
        }

        return bars;
    }

    void BarsRenderer::RenderBarShadows(
        Canvas& canvas,
        const std::vector<BarData>& bars
    ) const {
        RectBatch shadowBatches;
        const Color shadowColor = AdjustAlpha(
            Color::Black(),
            kShadowAlpha
        );

        for (const auto& bar : bars) {
            Rect shadowRect = bar.rect;
            shadowRect.x += kShadowOffsetX;
            shadowRect.y += kShadowOffsetY;

            shadowBatches[shadowColor].push_back(shadowRect);
        }

        RenderRectBatches(
            canvas,
            shadowBatches,
            m_settings.cornerRadius,
            RoundingMode::Top
        );
    }

    void BarsRenderer::RenderBarBodies(
        Canvas& canvas,
        const std::vector<BarData>& bars
    ) const {
        RectBatch barBatches;

        for (const auto& bar : bars) {
            barBatches[bar.color].push_back(bar.rect);
        }

        RenderRectBatches(
            canvas,
            barBatches,
            m_settings.cornerRadius,
            RoundingMode::Top
        );
    }

    void BarsRenderer::RenderBarHighlights(
        Canvas& canvas,
        const std::vector<BarData>& bars
    ) const {
        for (const auto& bar : bars) {
            const Rect highlightRect = CalculateHighlightRect(bar.rect);

            const Color highlightColor = AdjustAlpha(
                Color::White(),
                kHighlightAlpha * bar.magnitude
            );

            DrawRoundedRect(
                canvas,
                highlightRect,
                m_settings.cornerRadius,
                Paint::Fill(highlightColor),
                RoundingMode::Top
            );
        }
    }

    Color BarsRenderer::CalculateBarColor(
        float magnitude
    ) const {
        const float brightness = kBrightnessMin +
            kBrightnessRange * magnitude;

        return AdjustBrightness(GetPrimaryColor(), brightness);
    }

    Rect BarsRenderer::CalculateHighlightRect(
        const Rect& barRect
    ) const {
        const float highlightHeight = barRect.height *
            kHighlightHeightRatio;

        return Rect{
            barRect.x,
            barRect.y,
            barRect.width,
            highlightHeight
        };
    }

} // namespace Spectrum