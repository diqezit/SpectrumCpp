// Implements the BarsRenderer class

#include "BarsRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "RenderUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor & Settings
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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
            // provide a safe fallback for unknown quality levels
            m_settings = { 2.0f, 3.0f, false, true };
            break;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Core Render Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BarsRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        const size_t barCount = spectrum.size();
        const auto bl = RenderUtils::ComputeBarLayout(
            barCount,
            m_settings.barSpacing,
            m_width
        );
        // skip rendering if bars are too small to be visible
        if (bl.barWidth <= 0.0f) return;

        for (size_t i = 0; i < barCount; ++i) {
            const float mag = spectrum[i];
            const float h = RenderUtils::MagnitudeToHeight(mag, m_height, 0.9f);
            // skip rendering bars with no height for performance
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Single Bar Rendering Steps (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BarsRenderer::RenderBar(
        GraphicsContext& context,
        const Rect& rect,
        float magnitude
    ) {
        Color barColor = CalculateBarColor(magnitude);

        RenderBarWithEffects(context, rect, barColor);
        RenderHighlight(context, rect, magnitude);
    }

    void BarsRenderer::RenderBarWithEffects(
        GraphicsContext& context,
        const Rect& rect,
        const Color& color
    ) {
        // use a lambda to avoid repeating the main draw call
        auto drawCall = [&]() {
            RenderMainBar(context, rect, color);
            };

        // delegate shadow rendering to GraphicsContext
        if (m_settings.useShadow) {
            context.DrawWithShadow(
                drawCall,
                { 2.0f, 2.0f },
                0.0f, // blur is unused
                Color(0, 0, 0, 0.3f)
            );
        }
        else {
            drawCall();
        }
    }

    void BarsRenderer::RenderMainBar(
        GraphicsContext& context,
        const Rect& rect,
        const Color& color
    ) {
        // draw rounded corners only if radius is meaningful
        if (m_settings.cornerRadius > 0.0f) {
            context.DrawRoundedRectangle(
                rect,
                m_settings.cornerRadius,
                color,
                true
            );
        }
        else {
            context.DrawRectangle(rect, color, true);
        }
    }

    void BarsRenderer::RenderHighlight(
        GraphicsContext& context,
        const Rect& rect,
        float magnitude
    ) {
        // add highlight only if enabled to avoid unnecessary draw calls
        if (!m_settings.useHighlight) return;

        // highlight should be small and near the top for a 3D effect
        Rect hl(
            rect.x + 2.0f,
            rect.y + 2.0f,
            rect.width - 4.0f,
            std::min(10.0f, rect.height * 0.2f)
        );

        // highlight must be valid geometry
        if (hl.width <= 0 || hl.height <= 0) return;

        context.DrawRectangle(hl, Color(1, 1, 1, 0.2f * magnitude), true);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color BarsRenderer::CalculateBarColor(float magnitude) const {
        // vary bar brightness by magnitude for visual feedback
        return Utils::AdjustBrightness(
            m_primaryColor,
            0.7f + 0.6f * magnitude
        );
    }
}