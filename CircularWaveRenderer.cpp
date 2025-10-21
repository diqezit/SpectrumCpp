// CircularWaveRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the CircularWaveRenderer for concentric ring visualizations.
//
// Implementation details:
// - Rotation speed modulated by audio intensity
// - Sine wave offset creates fluid, organic motion
// - Glow rendered behind rings (back-to-front order)
// - Rings fade out as they move away from center
// - Uses D2DHelpers constants and utilities
//
// Rendering order:
// 1. Iterate rings from back to front (correct alpha blending)
// 2. Skip invisible rings (magnitude < threshold)
// 3. Render glow (if enabled and magnitude > threshold)
// 4. Render main ring
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "CircularWaveRenderer.h"
#include "D2DHelpers.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "RenderUtils.h"

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kCenterRadius = 30.0f;
        constexpr float kMaxRadiusFactor = 0.45f;
        constexpr float kMinStroke = 1.5f;
        constexpr float kStrokeClampFactor = 6.0f;
        constexpr float kWaveInfluence = 1.0f;
        constexpr float kWavePhaseOffset = 0.1f;
        constexpr float kRotationIntensityFactor = 0.3f;
        constexpr float kMinMagnitudeThreshold = 0.01f;
        constexpr float kGlowThreshold = 0.5f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    CircularWaveRenderer::CircularWaveRenderer()
        : m_angle(0.0f)
        , m_waveTime(0.0f)
    {
        m_primaryColor = Color::FromRGB(0, 150, 255);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::UpdateSettings()
    {
        if (m_isOverlay) {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = { false, 4.0f, 12, 0.4f, 1.5f };
                break;
            case RenderQuality::High:
                m_settings = { true, 6.0f, 20, 0.4f, 1.5f };
                break;
            default:
                m_settings = { true, 5.0f, 16, 0.4f, 1.5f };
                break;
            }
        }
        else {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = { false, 6.0f, 16, 0.5f, 2.0f };
                break;
            case RenderQuality::High:
                m_settings = { true, 8.0f, 32, 0.5f, 2.0f };
                break;
            default:
                m_settings = { true, 7.0f, 24, 0.5f, 2.0f };
                break;
            }
        }
    }

    void CircularWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        const float avgIntensity = RenderUtils::GetAverageMagnitude(spectrum);

        m_angle += m_settings.rotationSpeed
            * (1.0f + avgIntensity * kRotationIntensityFactor)
            * deltaTime;

        if (m_angle > kTwoPi) {
            m_angle -= kTwoPi;
        }

        m_waveTime += m_settings.waveSpeed * deltaTime;
    }

    void CircularWaveRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    )
    {
        const Point center = { m_width * 0.5f, m_height * 0.5f };
        const float maxRadius = std::min(m_width, m_height) * kMaxRadiusFactor;

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

            RenderCalculatedRing(context, center, radius, magnitude, distanceFactor);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Single Ring Rendering (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::RenderCalculatedRing(
        GraphicsContext& context,
        const Point& center,
        float radius,
        float magnitude,
        float distanceFactor
    ) const
    {
        const float strokeWidth = CalculateStrokeWidth(magnitude);
        const Color ringColor = CalculateRingColor(magnitude, distanceFactor);

        RenderGlowForRing(context, center, radius, strokeWidth, magnitude, ringColor);
        RenderMainRing(context, center, radius, strokeWidth, ringColor);
    }

    void CircularWaveRenderer::RenderGlowForRing(
        GraphicsContext& context,
        const Point& center,
        float radius,
        float strokeWidth,
        float magnitude,
        const Color& baseColor
    ) const
    {
        if (!m_settings.useGlow || magnitude <= kGlowThreshold) return;

        Color glowColor = baseColor;
        glowColor.a *= 0.5f;

        const float glowRadius = radius + strokeWidth;
        context.DrawGlow(center, glowRadius, glowColor, magnitude * 0.8f);
    }

    void CircularWaveRenderer::RenderMainRing(
        GraphicsContext& context,
        const Point& center,
        float radius,
        float strokeWidth,
        const Color& color
    ) const
    {
        const float innerRadius = radius - strokeWidth * 0.5f;
        const float outerRadius = radius + strokeWidth * 0.5f;

        if (innerRadius < outerRadius && innerRadius > 0.0f) {
            context.DrawRing(center, innerRadius, outerRadius, color);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color CircularWaveRenderer::CalculateRingColor(
        float magnitude,
        float distanceFactor
    ) const
    {
        const float alpha = Utils::Saturate(magnitude * 1.5f * distanceFactor);

        Color ringColor = m_primaryColor;
        ringColor.a = alpha;

        return ringColor;
    }

    float CircularWaveRenderer::CalculateRingRadius(
        int index,
        float ringStep,
        float magnitude
    ) const
    {
        const float baseRadius = kCenterRadius + index * ringStep;
        const float waveOffset = std::sin(
            m_waveTime + index * kWavePhaseOffset + m_angle
        ) * magnitude * ringStep * kWaveInfluence;

        return baseRadius + waveOffset;
    }

    float CircularWaveRenderer::CalculateStrokeWidth(float magnitude) const
    {
        return Utils::Clamp(
            kMinStroke + magnitude * kStrokeClampFactor,
            kMinStroke,
            m_settings.maxStroke
        );
    }

    float CircularWaveRenderer::GetRingMagnitude(
        const SpectrumData& spectrum,
        int ringIndex,
        int ringCount
    )
    {
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