// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BarsRenderer.cpp: Implementation of the BarsRenderer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BarsRenderer.h"
#include "Utils.h"

namespace Spectrum {

    BarsRenderer::BarsRenderer() {
        m_primaryColor = Color::FromRGB(33, 150, 243);
        UpdateSettings();
    }

    void BarsRenderer::UpdateSettings() {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { 1.0f, 0.0f, false, false };
            break;
        case RenderQuality::Medium:
            m_settings = { 2.0f, 3.0f, false, true };
            break;
        case RenderQuality::High:
            m_settings = { 2.0f, 5.0f, true, true };
            break;
        default:
            m_settings = { 2.0f, 3.0f, false, true };
            break;
        }
    }

    void BarsRenderer::DoRender(GraphicsContext& context,
        const SpectrumData& spectrum) {
        const size_t barCount = spectrum.size();
        const auto bl = ComputeBarLayout(barCount, m_settings.barSpacing);
        if (bl.barWidth <= 0.0f) return;

        for (size_t i = 0; i < barCount; ++i) {
            const float mag = spectrum[i];
            const float h = MagnitudeToHeight(mag, 0.9f);
            if (h < 1.0f) continue;

            const Rect rect(
                i * bl.totalBarWidth + bl.spacing * 0.5f,
                static_cast<float>(m_height) - h,
                bl.barWidth,
                h
            );
            RenderBar(context, rect, mag);
        }
    }

    void BarsRenderer::RenderBar(GraphicsContext& context,
        const Rect& rect,
        float magnitude) {
        Color barColor = Utils::AdjustBrightness(
            m_primaryColor, 0.7f + 0.6f * magnitude
        );

        if (m_settings.useShadow) {
            Rect shadow(rect.x + 2.0f, rect.y + 2.0f,
                rect.width, rect.height);
            context.DrawRoundedRectangle(
                shadow, m_settings.cornerRadius,
                Color(0, 0, 0, 0.3f), true
            );
        }

        if (m_settings.cornerRadius > 0.0f) {
            context.DrawRoundedRectangle(
                rect, m_settings.cornerRadius, barColor, true
            );
        }
        else {
            context.DrawRectangle(rect, barColor, true);
        }

        if (m_settings.useHighlight) {
            Rect hl(
                rect.x + 2.0f,
                rect.y + 2.0f,
                rect.width - 4.0f,
                std::min(10.0f, rect.height * 0.2f)
            );
            context.DrawRectangle(
                hl, Color(1, 1, 1, 0.2f * magnitude), true
            );
        }
    }

} // namespace Spectrum