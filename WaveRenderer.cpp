#include "WaveRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WaveRenderer::WaveRenderer() {
        m_primaryColor = Color::FromRGB(100, 255, 100);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::UpdateSettings() {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { 1.5f, false, false };
            break;
        case RenderQuality::Medium:
            m_settings = { 2.0f, false, true };
            break;
        case RenderQuality::High:
            m_settings = { 2.5f, true, true };
            break;
        default:
            m_settings = { 2.0f, false, true };
            break;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        Rect bounds(
            0.f,
            0.f,
            static_cast<float>(m_width),
            static_cast<float>(m_height)
        );

        if (m_settings.useGlow) {
            DrawGlowEffect(
                context,
                spectrum,
                bounds
            );
        }

        DrawMainWaveform(
            context,
            spectrum,
            bounds
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::DrawGlowEffect(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const {
        const int glowLayers = 4;
        for (int i = 1; i <= glowLayers; ++i) {
            Color glowColor = m_primaryColor;
            glowColor.a *= 0.3f / i;
            float glowWidth = m_settings.lineWidth + i * 2.0f;

            DrawGlowLayer(
                context,
                spectrum,
                bounds,
                glowColor,
                glowWidth
            );
        }
    }

    void WaveRenderer::DrawGlowLayer(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& glowColor,
        float glowWidth
    ) const {
        context.DrawWaveform(
            spectrum,
            bounds,
            glowColor,
            glowWidth,
            false // mirror: false
        );

        if (m_settings.useReflection) {
            Color reflectionGlowColor = glowColor;
            reflectionGlowColor.a *= 0.5f;

            context.DrawWaveform(
                spectrum,
                bounds,
                reflectionGlowColor,
                glowWidth,
                true // mirror: true
            );
        }
    }

    void WaveRenderer::DrawMainWaveform(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const {
        context.DrawWaveform(
            spectrum,
            bounds,
            m_primaryColor,
            m_settings.lineWidth,
            false // mirror: false
        );

        if (m_settings.useReflection) {
            Color reflectionColor = m_primaryColor;
            reflectionColor.a *= 0.4f;

            context.DrawWaveform(
                spectrum,
                bounds,
                reflectionColor,
                m_settings.lineWidth,
                true // mirror: true
            );
        }
    }

} // namespace Spectrum