#include "Graphics/Visualizers/CircularWaveRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    namespace {
        constexpr float kCenterRadius = 30.0f;
        constexpr float kWaveInfluence = 1.0f;
        constexpr float kWavePhaseOffset = 0.1f;
        constexpr float kRotationIntensityFactor = 0.3f;
        constexpr float kRotationSpeedBase = 1.0f;
        constexpr float kMinMagnitudeThreshold = 0.01f;
        constexpr float kGlowThreshold = 0.5f;
        constexpr float kAlphaMultiplier = 1.5f;
        constexpr float kGlowAlphaFactor = 0.5f;
        constexpr float kGlowIntensityFactor = 0.8f;
        constexpr float kMinStroke = 1.5f;
        constexpr float kStrokeMultiplier = 6.0f;
    }

    CircularWaveRenderer::CircularWaveRenderer()
        : m_angle(0.0f)
        , m_waveTime(0.0f)
    {
        UpdateSettings();
    }

    void CircularWaveRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
    }

    void CircularWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        const float avgIntensity = RenderUtils::GetAverageMagnitude(spectrum);
        const float rotationSpeed = m_settings.rotationSpeed *
            (kRotationSpeedBase + avgIntensity * kRotationIntensityFactor);

        m_angle += rotationSpeed * deltaTime;
        if (m_angle > TWO_PI) m_angle -= TWO_PI;

        m_waveTime += m_settings.waveSpeed * deltaTime;
    }

    void CircularWaveRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        const Point center = GetViewportCenter();
        const float maxRadius = GetMaxRadius();
        const int ringCount = std::min(
            static_cast<int>(spectrum.size()),
            m_settings.maxRings
        );

        if (ringCount == 0) return;

        const float ringStep = (maxRadius - kCenterRadius) / ringCount;

        for (int i = ringCount - 1; i >= 0; --i) {
            const float magnitude = GetRingMagnitude(spectrum, i, ringCount);
            if (magnitude < kMinMagnitudeThreshold) continue;

            const float radius = CalculateRingRadius(i, ringStep, magnitude);
            if (radius <= 0.0f || radius > maxRadius) continue;

            const float distanceFactor = 1.0f - radius / maxRadius;
            const float strokeWidth = std::clamp(
                kMinStroke + magnitude * kStrokeMultiplier,
                kMinStroke,
                m_settings.maxStroke
            );

            const float alpha = Helpers::Math::Saturate(
                magnitude * kAlphaMultiplier * distanceFactor
            );

            const Color ringColor = AdjustAlpha(GetPrimaryColor(), alpha);

            if (m_settings.useGlow && magnitude > kGlowThreshold) {
                const Color glowColor = AdjustAlpha(
                    ringColor,
                    ringColor.a * kGlowAlphaFactor
                );

                canvas.DrawGlow(
                    center,
                    radius + strokeWidth,
                    glowColor,
                    magnitude * kGlowIntensityFactor
                );
            }

            const float innerRadius = radius - strokeWidth * 0.5f;
            const float outerRadius = radius + strokeWidth * 0.5f;

            if (innerRadius > 0.0f && innerRadius < outerRadius) {
                canvas.DrawRing(
                    center,
                    innerRadius,
                    outerRadius,
                    Paint::Stroke(ringColor)
                );
            }
        }
    }

    float CircularWaveRenderer::CalculateRingRadius(
        int index,
        float ringStep,
        float magnitude
    ) const {
        const float baseRadius = kCenterRadius + index * ringStep;
        const float waveOffset = std::sin(
            m_waveTime + index * kWavePhaseOffset + m_angle
        ) * magnitude * ringStep * kWaveInfluence;

        return baseRadius + waveOffset;
    }

    float CircularWaveRenderer::GetRingMagnitude(
        const SpectrumData& spectrum,
        int ringIndex,
        int ringCount
    ) {
        if (spectrum.empty() || ringCount == 0) return 0.0f;

        const size_t n = spectrum.size();
        const size_t start = (static_cast<size_t>(ringIndex) * n) / ringCount;
        const size_t end = std::min(
            ((static_cast<size_t>(ringIndex) + 1) * n) / ringCount,
            n
        );

        return RenderUtils::AverageRange(spectrum, start, end);
    }

} // namespace Spectrum