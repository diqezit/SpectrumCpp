#include "Graphics/Visualizers/WaveRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Math;
    using namespace Helpers::Geometry;

    namespace {
        constexpr float kGlowAlphaBase = 0.4f;
        constexpr float kGlowWidthIncrement = 2.5f;
        constexpr float kGlowIntensityBoost = 1.2f;
        constexpr float kReflectionAlpha = 0.45f;
        constexpr float kReflectionGlowAlpha = 0.55f;
        constexpr float kIntensitySmoothing = 0.15f;
        constexpr float kHighIntensityThreshold = 0.7f;
        constexpr float kBrightnessBoostMax = 1.3f;
        constexpr float kLineWidthBase = 2.0f;
    }

    WaveRenderer::WaveRenderer()
        : m_smoothedIntensity(0.0f)
    {
        UpdateSettings();
    }

    void WaveRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
    }

    void WaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        if (spectrum.empty()) return;

        const float targetIntensity = RenderUtils::GetAverageMagnitude(spectrum);
        const float smoothing = Clamp(
            kIntensitySmoothing * deltaTime * 60.0f,
            0.0f,
            1.0f
        );
        m_smoothedIntensity = Lerp(
            m_smoothedIntensity,
            targetIntensity,
            smoothing
        );
    }

    void WaveRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        if (spectrum.empty()) return;

        const Rect bounds = GetViewportBounds();
        const float lineWidth = GetLineWidth();
        Color mainColor = GetPrimaryColor();

        // Apply brightness boost for high intensity
        if (m_smoothedIntensity > kHighIntensityThreshold) {
            const float intensityRatio = MapToRange(
                m_smoothedIntensity,
                kHighIntensityThreshold,
                1.0f,
                0.0f,
                1.0f
            );
            const float boost = Lerp(1.0f, kBrightnessBoostMax, intensityRatio);
            mainColor = AdjustBrightness(mainColor, boost);
        }

        // Render shadow
        if (m_settings.useFill && m_settings.useMirror) {
            auto drawShadow = [&]() {
                RenderWaveform(canvas, spectrum, bounds, mainColor, lineWidth, false);
                };
            RenderWithShadow(canvas, drawShadow, { 0.0f, 3.0f }, 0.5f);
        }

        // Render glow layers
        if (m_settings.useFill) {
            const int layerCount = GetGlowLayerCount();
            for (int i = layerCount; i >= 1; --i) {
                const Color glowColor = CalculateGlowColor(i);
                const float glowWidth = CalculateGlowWidth(i);

                RenderWaveform(canvas, spectrum, bounds, glowColor, glowWidth, false);

                if (m_settings.useMirror) {
                    Color reflectionGlowColor = glowColor;
                    reflectionGlowColor.a *= kReflectionGlowAlpha;
                    RenderWaveform(
                        canvas,
                        spectrum,
                        bounds,
                        reflectionGlowColor,
                        glowWidth,
                        true
                    );
                }
            }
        }

        // Render main waveform
        RenderWaveform(canvas, spectrum, bounds, mainColor, lineWidth, false);

        // Render reflection
        if (m_settings.useMirror) {
            Color reflectionColor = mainColor;
            reflectionColor.a *= kReflectionAlpha;
            RenderWaveform(canvas, spectrum, bounds, reflectionColor, lineWidth, true);
        }
    }

    void WaveRenderer::RenderWaveform(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& color,
        float width,
        bool reflected
    ) const {
        Paint paint = Paint::Stroke(color, width)
            .WithStrokeCap(StrokeCap::Round)
            .WithStrokeJoin(StrokeJoin::Round);
        canvas.DrawWaveform(spectrum, bounds, paint, reflected);
    }

    Color WaveRenderer::CalculateGlowColor(int layerIndex) const {
        Color glowColor = GetPrimaryColor();
        const float baseAlpha = kGlowAlphaBase / layerIndex;
        const int totalLayers = GetGlowLayerCount();
        const float layerRatio = Normalize(
            static_cast<float>(layerIndex),
            0.0f,
            static_cast<float>(totalLayers)
        );
        const float alpha = baseAlpha * (1.0f + layerRatio * 0.5f);
        const float intensityMultiplier = Lerp(
            1.0f,
            kGlowIntensityBoost,
            m_smoothedIntensity
        );

        glowColor.a *= alpha * m_settings.smoothness * intensityMultiplier;
        return glowColor;
    }

    float WaveRenderer::CalculateGlowWidth(int layerIndex) const {
        return GetLineWidth() + layerIndex * kGlowWidthIncrement;
    }

    int WaveRenderer::GetGlowLayerCount() const {
        return m_settings.points / 64;
    }

    float WaveRenderer::GetLineWidth() const {
        return kLineWidthBase * m_settings.waveHeight;
    }

} // namespace Spectrum