// WaveRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WaveRenderer for smooth waveform visualization.
//
// Implementation details:
// - Quality settings control line width, glow, and reflection
// - Glow rendered as multiple passes with decreasing alpha
// - Reflection rendered below main waveform with reduced alpha
// - All drawing delegated to GraphicsContext.DrawWaveform
//
// Rendering pipeline:
// 1. Draw glow layers (if enabled, back to front)
// 2. Draw main waveform
// 3. Draw reflection (if enabled)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WaveRenderer.h"
#include "D2DHelpers.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr int kGlowLayerCount = 4;
        constexpr float kGlowAlphaBase = 0.3f;
        constexpr float kGlowWidthIncrement = 2.0f;
        constexpr float kReflectionAlpha = 0.4f;
        constexpr float kReflectionGlowAlpha = 0.5f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WaveRenderer::WaveRenderer()
    {
        m_primaryColor = Color::FromRGB(100, 255, 100);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { 1.5f, false, false };
            break;
        case RenderQuality::High:
            m_settings = { 2.5f, true, true };
            break;
        case RenderQuality::Medium:
        default:
            m_settings = { 2.0f, false, true };
            break;
        }
    }

    void WaveRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    )
    {
        const Rect bounds{
            0.0f,
            0.0f,
            static_cast<float>(m_width),
            static_cast<float>(m_height)
        };

        if (m_settings.useGlow) {
            DrawGlowEffect(context, spectrum, bounds);
        }

        DrawMainWaveform(context, spectrum, bounds);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::DrawGlowEffect(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        for (int i = 1; i <= kGlowLayerCount; ++i) {
            Color glowColor = m_primaryColor;
            glowColor.a *= kGlowAlphaBase / i;

            const float glowWidth = m_settings.lineWidth + i * kGlowWidthIncrement;

            DrawGlowLayer(context, spectrum, bounds, glowColor, glowWidth);
        }
    }

    void WaveRenderer::DrawGlowLayer(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& glowColor,
        float glowWidth
    ) const
    {
        context.DrawWaveform(spectrum, bounds, glowColor, glowWidth, false);

        if (m_settings.useReflection) {
            Color reflectionGlowColor = glowColor;
            reflectionGlowColor.a *= kReflectionGlowAlpha;

            context.DrawWaveform(spectrum, bounds, reflectionGlowColor, glowWidth, true);
        }
    }

    void WaveRenderer::DrawMainWaveform(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        context.DrawWaveform(spectrum, bounds, m_primaryColor, m_settings.lineWidth, false);

        if (m_settings.useReflection) {
            Color reflectionColor = m_primaryColor;
            reflectionColor.a *= kReflectionAlpha;

            context.DrawWaveform(spectrum, bounds, reflectionColor, m_settings.lineWidth, true);
        }
    }

} // namespace Spectrum