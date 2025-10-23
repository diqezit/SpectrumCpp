// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the CircularWaveRenderer for concentric ring visualizations.
//
// Implementation details:
// - Rings positioned at regular intervals from center
// - Wave effect achieved through sine-based radius modulation
// - Rotation angle increases with audio intensity
// - Stroke width scales with magnitude for visual impact
// - Alpha decreases with distance for depth perception
// - Uses GeometryHelpers for all geometric operations
//
// Rendering pipeline:
// 1. Calculate effective ring count from spectrum size
// 2. Render rings back-to-front (largest to smallest)
// 3. For each ring:
//    a. Calculate magnitude from spectrum range
//    b. Apply wave offset to radius
//    c. Render glow layer (if enabled and magnitude high)
//    d. Render ring shape with calculated stroke
//
// Visual design:
// - Inner rings: brighter, thicker strokes
// - Outer rings: dimmer, thinner strokes
// - Wave creates pulsing motion
// - Rotation creates hypnotic spiral effect
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/CircularWaveRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Geometry;

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
        constexpr float kRotationSpeedBase = 1.0f;

        constexpr float kMinMagnitudeThreshold = 0.01f;
        constexpr float kGlowThreshold = 0.5f;

        constexpr float kAlphaMultiplier = 1.5f;
        constexpr float kGlowAlphaFactor = 0.5f;
        constexpr float kGlowIntensityFactor = 0.8f;

        constexpr float kStrokeOffsetFactor = 0.5f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    CircularWaveRenderer::CircularWaveRenderer()
        : BaseRenderer(),
        m_angle(0.0f),
        m_waveTime(0.0f)
    {
        m_primaryColor = Color::FromRGB(0, 150, 255);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<CircularWaveRenderer>(m_quality, m_isOverlay);
    }

    void CircularWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        const float avgIntensity = RenderUtils::GetAverageMagnitude(spectrum);

        UpdateRotationAngle(avgIntensity, deltaTime);
        UpdateWavePhase(deltaTime);
    }

    void CircularWaveRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        const Point center = GetViewportCenter();
        const float maxRadius = GetMaxRadius();
        const int ringCount = GetEffectiveRingCount(spectrum);

        if (ringCount == 0) return;

        const float ringStep = CalculateRingStep(maxRadius, ringCount);

        for (int i = ringCount - 1; i >= 0; --i) {
            const float magnitude = GetRingMagnitude(spectrum, i, ringCount);

            if (!IsRingVisible(magnitude)) continue;

            const float radius = CalculateRingRadius(i, ringStep, magnitude);

            if (!IsRingInBounds(radius, maxRadius)) continue;

            const float distanceFactor = CalculateDistanceFactor(radius, maxRadius);

            RenderRing(canvas, center, radius, magnitude, distanceFactor);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::RenderRing(
        Canvas& canvas,
        const Point& center,
        float radius,
        float magnitude,
        float distanceFactor
    ) const
    {
        const float strokeWidth = CalculateStrokeWidth(magnitude);
        const Color ringColor = CalculateRingColor(magnitude, distanceFactor);

        RenderRingGlow(canvas, center, radius, strokeWidth, magnitude, ringColor);
        RenderRingShape(canvas, center, radius, strokeWidth, ringColor);
    }

    void CircularWaveRenderer::RenderRingGlow(
        Canvas& canvas,
        const Point& center,
        float radius,
        float strokeWidth,
        float magnitude,
        const Color& baseColor
    ) const
    {
        if (!ShouldRenderGlow(magnitude)) return;

        const Color glowColor = CalculateGlowColor(baseColor);
        const float glowRadius = radius + strokeWidth;

        canvas.DrawGlow(
            center,
            glowRadius,
            glowColor,
            magnitude * kGlowIntensityFactor
        );
    }

    void CircularWaveRenderer::RenderRingShape(
        Canvas& canvas,
        const Point& center,
        float radius,
        float strokeWidth,
        const Color& color
    ) const
    {
        const auto [innerRadius, outerRadius] = GetRingRadii(radius, strokeWidth);

        if (!CanRenderRingShape(innerRadius, outerRadius)) return;

        canvas.DrawRing(
            center,
            innerRadius,
            outerRadius,
            Paint::Stroke(color)
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::UpdateRotationAngle(
        float avgIntensity,
        float deltaTime
    )
    {
        const float rotationSpeed = m_settings.rotationSpeed
            * (kRotationSpeedBase + avgIntensity * kRotationIntensityFactor);

        m_angle += rotationSpeed * deltaTime;

        if (m_angle > TWO_PI) {
            m_angle -= TWO_PI;
        }
    }

    void CircularWaveRenderer::UpdateWavePhase(float deltaTime)
    {
        m_waveTime += m_settings.waveSpeed * deltaTime;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Point CircularWaveRenderer::GetViewportCenter() const
    {
        return Helpers::Geometry::GetViewportCenter(m_width, m_height);
    }

    float CircularWaveRenderer::GetMaxRadius() const
    {
        return Helpers::Geometry::GetMaxRadiusInViewport(m_width, m_height) * kMaxRadiusFactor;
    }

    float CircularWaveRenderer::CalculateRingStep(
        float maxRadius,
        int ringCount
    ) const
    {
        return (maxRadius - kCenterRadius) / ringCount;
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

    float CircularWaveRenderer::CalculateDistanceFactor(
        float radius,
        float maxRadius
    ) const
    {
        return 1.0f - radius / maxRadius;
    }

    float CircularWaveRenderer::CalculateStrokeWidth(float magnitude) const
    {
        return Helpers::Math::Clamp(
            kMinStroke + magnitude * kStrokeClampFactor,
            kMinStroke,
            m_settings.maxStroke
        );
    }

    std::pair<float, float> CircularWaveRenderer::GetRingRadii(
        float centerRadius,
        float strokeWidth
    ) const
    {
        const float innerRadius = centerRadius - strokeWidth * kStrokeOffsetFactor;
        const float outerRadius = centerRadius + strokeWidth * kStrokeOffsetFactor;

        return { innerRadius, outerRadius };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color CircularWaveRenderer::CalculateRingColor(
        float magnitude,
        float distanceFactor
    ) const
    {
        const float alpha = Helpers::Math::Saturate(
            magnitude * kAlphaMultiplier * distanceFactor
        );

        return m_primaryColor.WithAlpha(alpha);
    }

    Color CircularWaveRenderer::CalculateGlowColor(const Color& baseColor) const
    {
        Color glowColor = baseColor;
        glowColor.a *= kGlowAlphaFactor;
        return glowColor;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Data Processing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    int CircularWaveRenderer::GetEffectiveRingCount(const SpectrumData& spectrum) const
    {
        return std::min(
            static_cast<int>(spectrum.size()),
            m_settings.maxRings
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool CircularWaveRenderer::IsRingVisible(float magnitude) const
    {
        return magnitude >= kMinMagnitudeThreshold;
    }

    bool CircularWaveRenderer::IsRingInBounds(
        float radius,
        float maxRadius
    ) const
    {
        return radius > 0.0f && radius <= maxRadius;
    }

    bool CircularWaveRenderer::CanRenderRingShape(
        float innerRadius,
        float outerRadius
    ) const
    {
        return innerRadius > 0.0f && innerRadius < outerRadius;
    }

    bool CircularWaveRenderer::ShouldRenderGlow(float magnitude) const
    {
        return m_settings.useGlow && magnitude > kGlowThreshold;
    }

} // namespace Spectrum