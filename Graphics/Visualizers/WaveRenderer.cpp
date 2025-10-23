// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WaveRenderer with enhanced visual quality.
//
// Implementation details:
// - Quality settings control line width, glow layers, shadow blur
// - Multi-layer glow with intensity-based modulation
// - Shadow rendering with configurable offset and blur
// - Reflection rendered below main waveform with reduced alpha
// - Dynamic brightness boost during high-intensity audio
// - Smooth intensity transitions via exponential smoothing
// - Uses GeometryHelpers for all geometric operations
//
// Rendering pipeline:
// 1. Draw shadow layer (if enabled, with blur)
// 2. Draw glow layers (if enabled, back to front)
// 3. Draw main waveform with optional brightness boost
// 4. Draw reflection (if enabled)
//
// Visual enhancements:
// - Antialiased lines with rounded caps and joins
// - Progressive glow alpha based on layer depth
// - Intensity-responsive glow brightness
// - Smooth color transitions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/WaveRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"

namespace Spectrum {

    using namespace Helpers::Geometry;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kGlowAlphaBase = 0.4f;
        constexpr float kGlowWidthIncrement = 2.5f;
        constexpr float kGlowIntensityBoost = 1.2f;

        constexpr float kReflectionAlpha = 0.45f;
        constexpr float kReflectionGlowAlpha = 0.55f;

        constexpr float kShadowOffsetX = 0.0f;
        constexpr float kShadowOffsetY = 3.0f;
        constexpr float kShadowAlpha = 0.5f;
        constexpr float kShadowBlurBase = 4.0f;
        constexpr float kShadowBlurScale = 2.0f;

        constexpr float kIntensitySmoothing = 0.15f;
        constexpr float kHighIntensityThreshold = 0.7f;
        constexpr float kBrightnessBoostMax = 1.3f;

        constexpr float kLineWidthBase = 2.0f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WaveRenderer::WaveRenderer()
        : m_smoothedIntensity(0.0f)
    {
        m_primaryColor = Color::FromRGB(100, 255, 100);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WaveRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<WaveRenderer>(m_quality);
    }

    void WaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        if (spectrum.empty()) return;

        const float targetIntensity = RenderUtils::GetAverageMagnitude(spectrum);

        const float smoothing = Helpers::Math::Clamp(
            kIntensitySmoothing * deltaTime * 60.0f,
            0.0f,
            1.0f
        );

        m_smoothedIntensity = Helpers::Math::Lerp(
            m_smoothedIntensity,
            targetIntensity,
            smoothing
        );
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
        if (ShouldRenderShadow()) {
            RenderShadowLayer(canvas, spectrum, bounds);
        }

        if (ShouldRenderGlow()) {
            RenderGlowEffect(canvas, spectrum, bounds);
        }

        RenderMainWaveform(canvas, spectrum, bounds);
    }

    void WaveRenderer::RenderShadowLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        const Color shadowColor = CalculateShadowColor();

        auto drawShadow = [&]() {
            RenderWaveform(
                canvas,
                spectrum,
                bounds,
                m_primaryColor,
                GetLineWidth(),
                false
            );
            };

        canvas.DrawWithShadow(
            drawShadow,
            { kShadowOffsetX, kShadowOffsetY },
            GetShadowBlur(),
            shadowColor
        );
    }

    void WaveRenderer::RenderGlowEffect(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds
    ) const
    {
        const int layerCount = GetGlowLayerCount();

        for (int i = layerCount; i >= 1; --i) {
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
        Color mainColor = m_primaryColor;

        if (m_smoothedIntensity > kHighIntensityThreshold) {

            const float intensityRatio = Helpers::Math::Map(
                m_smoothedIntensity,
                kHighIntensityThreshold,
                1.0f,
                0.0f,
                1.0f
            );

            const float boost = Helpers::Math::Lerp(
                1.0f,
                kBrightnessBoostMax,
                intensityRatio
            );

            mainColor = Helpers::Color::AdjustBrightness(mainColor, boost);
        }

        RenderWaveform(
            canvas,
            spectrum,
            bounds,
            mainColor,
            GetLineWidth(),
            false
        );

        if (ShouldRenderReflection()) {
            RenderReflection(
                canvas,
                spectrum,
                bounds,
                mainColor,
                GetLineWidth()
            );
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
        Paint paint = Paint::Stroke(color, width)
            .WithStrokeCap(StrokeCap::Round)
            .WithStrokeJoin(StrokeJoin::Round);

        canvas.DrawWaveform(spectrum, bounds, paint, reflected);
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
        return CreateViewportBounds(m_width, m_height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color WaveRenderer::CalculateGlowColor(int layerIndex) const
    {
        Color glowColor = m_primaryColor;

        const float baseAlpha = CalculateGlowAlpha(layerIndex);

        const float intensityMultiplier = Helpers::Math::Lerp(
            1.0f,
            kGlowIntensityBoost,
            m_smoothedIntensity
        );

        glowColor.a *= baseAlpha * m_settings.smoothness * intensityMultiplier;

        return glowColor;
    }

    Color WaveRenderer::CalculateReflectionColor(const Color& baseColor) const
    {
        Color reflectionColor = baseColor;
        reflectionColor.a *= GetGlowReflectionAlpha();

        return reflectionColor;
    }

    Color WaveRenderer::CalculateShadowColor() const
    {
        return Color(0.0f, 0.0f, 0.0f, kShadowAlpha);
    }

    float WaveRenderer::CalculateGlowAlpha(int layerIndex) const
    {
        const float baseAlpha = kGlowAlphaBase / layerIndex;
        const int totalLayers = GetGlowLayerCount();

        const float layerRatio = Helpers::Math::Normalize(
            static_cast<float>(layerIndex),
            0.0f,
            static_cast<float>(totalLayers)
        );

        return baseAlpha * (1.0f + layerRatio * 0.5f);
    }

    float WaveRenderer::CalculateGlowWidth(int layerIndex) const
    {
        return GetLineWidth() + layerIndex * kGlowWidthIncrement;
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
        return m_settings.points / 64;  // Dynamic based on points
    }

    float WaveRenderer::GetLineWidth() const
    {
        return kLineWidthBase * m_settings.waveHeight;
    }

    float WaveRenderer::GetShadowBlur() const
    {
        return kShadowBlurBase + m_settings.smoothness * kShadowBlurScale;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WaveRenderer::ShouldRenderGlow() const
    {
        return m_settings.useFill;
    }

    bool WaveRenderer::ShouldRenderReflection() const
    {
        return m_settings.useMirror;
    }

    bool WaveRenderer::ShouldRenderShadow() const
    {
        return m_settings.useFill && m_settings.useMirror;
    }

    bool WaveRenderer::IsSpectrumValid(const SpectrumData& spectrum) const
    {
        return !spectrum.empty();
    }

} // namespace Spectrum