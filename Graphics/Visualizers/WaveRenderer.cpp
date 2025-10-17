// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WaveRenderer for smooth waveform visualization.
//
// Implementation details:
// - Quality settings control line width, glow, and reflection
// - Glow rendered as multiple passes with decreasing alpha
// - Reflection rendered below main waveform with reduced alpha
// - All drawing delegated to Canvas.DrawWaveform
//
// Rendering pipeline:
// 1. Draw glow layers (if enabled, back to front)
// 2. Draw main waveform
// 3. Draw reflection (if enabled)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/WaveRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"

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
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        if (!IsSpectrumValid(spectrum)) return;

        const Rect bounds = GetRenderBounds();

        RenderAllLayers(canvas, spectrum, bounds);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::RenderAllLayers(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        if (ShouldRenderGlow()) {
            RenderGlowEffect(canvas, spectrum, bounds);
        }

        RenderMainWaveform(canvas, spectrum, bounds);
    }

    void WaveRenderer::RenderGlowEffect(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        const int layerCount = GetGlowLayerCount();

        for (int i = 1; i <= layerCount; ++i) {
            RenderGlowLayer(canvas, spectrum, bounds, i);
        }
    }

    void WaveRenderer::RenderGlowLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds,
        int layerIndex
    ) const
    {
        const Color glowColor = CalculateGlowColor(layerIndex);
        const float glowWidth = CalculateGlowWidth(layerIndex);

        RenderWaveform(canvas, spectrum, bounds, glowColor, glowWidth, false);

        if (ShouldRenderReflection()) {
            const Color reflectionGlowColor = CalculateReflectionColor(glowColor);
            RenderWaveform(canvas, spectrum, bounds, reflectionGlowColor, glowWidth, true);
        }
    }

    void WaveRenderer::RenderMainWaveform(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        RenderWaveform(
            canvas,
            spectrum,
            bounds,
            m_primaryColor,
            m_settings.lineWidth,
            false
        );

        if (ShouldRenderReflection()) {
            RenderReflection(canvas, spectrum, bounds, m_primaryColor, m_settings.lineWidth);
        }
    }

    void WaveRenderer::RenderWaveform(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& color,
        float width,
        bool reflected
    ) const
    {
        canvas.DrawWaveform(
            spectrum,
            bounds,
            Paint::Stroke(color, width),
            reflected
        );
    }

    void WaveRenderer::RenderReflection(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& baseColor,
        float width
    ) const
    {
        const Color reflectionColor = CalculateReflectionColor(baseColor);

        RenderWaveform(canvas, spectrum, bounds, reflectionColor, width, true);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Rect WaveRenderer::GetRenderBounds() const
    {
        return {
            0.0f,
            0.0f,
            static_cast<float>(m_width),
            static_cast<float>(m_height)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color WaveRenderer::CalculateGlowColor(int layerIndex) const
    {
        Color glowColor = m_primaryColor;
        glowColor.a *= CalculateGlowAlpha(layerIndex);

        return glowColor;
    }

    Color WaveRenderer::CalculateReflectionColor(const Color& baseColor) const
    {
        Color reflectionColor = baseColor;
        reflectionColor.a *= GetGlowReflectionAlpha();

        return reflectionColor;
    }

    float WaveRenderer::CalculateGlowAlpha(int layerIndex) const
    {
        return kGlowAlphaBase / layerIndex;
    }

    float WaveRenderer::CalculateGlowWidth(int layerIndex) const
    {
        return m_settings.lineWidth + layerIndex * kGlowWidthIncrement;
    }

    float WaveRenderer::GetReflectionAlpha() const
    {
        return kReflectionAlpha;
    }

    float WaveRenderer::GetGlowReflectionAlpha() const
    {
        return kReflectionGlowAlpha;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    int WaveRenderer::GetGlowLayerCount() const
    {
        return kGlowLayerCount;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WaveRenderer::ShouldRenderGlow() const
    {
        return m_settings.useGlow;
    }

    bool WaveRenderer::ShouldRenderReflection() const
    {
        return m_settings.useReflection;
    }

    bool WaveRenderer::IsSpectrumValid(const SpectrumData& spectrum) const
    {
        return !spectrum.empty();
    }

} // namespace Spectrum