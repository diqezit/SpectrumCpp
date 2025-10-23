// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the PolylineWaveRenderer with enhanced visual effects
//
// Advanced rendering features:
// - Gradient bars with smooth color transitions
// - Pulsating core synchronized with audio intensity
// - Multi-layer depth with glow and highlight effects
// - Dynamic color interpolation based on magnitude
// - Optimized rendering with pre-calculated directions
// - Uses GeometryHelpers for all geometric operations
//
// Visual design:
// - Base color fades to accent color along bar length
// - Core pulses in sync with average spectrum intensity
// - Highlights only appear on strong peaks
// - Outer glow creates depth perception
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/PolylineWaveRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kRadiusFactor = 0.7f;
        constexpr float kCoreRadiusMin = 0.08f;
        constexpr float kCoreRadiusMax = 0.15f;
        constexpr float kCoreRadiusSmoothing = 0.1f;

        constexpr float kBarLengthScale = 0.6f;
        constexpr float kMinBarLengthFactor = 0.05f;
        constexpr float kMinBarWidth = 2.0f;
        constexpr float kMaxBarWidth = 15.0f;
        constexpr float kBarSpacingRatio = 0.75f;

        constexpr float kMinMagnitude = 0.02f;
        constexpr float kGlowMagnitudeThreshold = 0.5f;
        constexpr float kHighlightMagnitudeThreshold = 0.7f;

        constexpr float kGlowBlurRadius = 8.0f;
        constexpr float kGlowAlpha = 0.6f;
        constexpr float kHighlightStartPosition = 0.6f;
        constexpr float kHighlightAlpha = 0.8f;

        constexpr float kCoreGlowAlpha = 0.4f;
        constexpr float kCoreStrokeWidth = 3.0f;
        constexpr float kOuterGlowStrokeMultiplier = 2.0f;

        constexpr int kGradientSegments = 5;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    PolylineWaveRenderer::PolylineWaveRenderer()
        : m_currentCoreRadius(0.0f)
        , m_targetCoreRadius(0.0f)
    {
        m_primaryColor = Color::FromRGB(0, 180, 255);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<PolylineWaveRenderer>(m_quality);

        m_currentCoreRadius = kCoreRadiusMin;
        m_targetCoreRadius = kCoreRadiusMin;
    }

    void PolylineWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        UpdateCoreRadius(spectrum, deltaTime);
    }

    void PolylineWaveRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        if (spectrum.empty()) return;

        EnsureBarDirections(spectrum.size());

        const Point center = GetViewportCenter();
        const float baseRadius = CalculateBaseRadius();
        const float barWidth = CalculateBarWidth(spectrum.size(), baseRadius);

        if (m_settings.useGlow) {
            RenderOuterGlow(canvas, spectrum, center, baseRadius, barWidth);
        }

        if (m_settings.useFill) {
            RenderPulsingCore(canvas, center, baseRadius);
        }

        if (m_settings.lineWidth > 0.0f) {
            RenderGradientBars(canvas, spectrum, center, baseRadius, barWidth);
        }
        else {
            RenderSolidBars(canvas, spectrum, center, baseRadius, barWidth);
        }

        if (m_settings.smoothness > kHighlightMagnitudeThreshold) {
            RenderDynamicHighlights(canvas, spectrum, center, baseRadius, barWidth);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::EnsureBarDirections(size_t barCount)
    {
        if (m_barDirections.size() != barCount) {
            m_barDirections.resize(barCount);

            const float angleStep = TWO_PI / barCount;

            for (size_t i = 0; i < barCount; ++i) {
                const float angle = i * angleStep;

                m_barDirections[i] = DirectionFromAngle(angle);
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::UpdateCoreRadius(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        if (!m_settings.useFill) {
            m_currentCoreRadius = kCoreRadiusMin;
            m_targetCoreRadius = kCoreRadiusMin;
            return;
        }

        const float avgIntensity = CalculateAverageIntensity(spectrum);

        m_targetCoreRadius = Helpers::Math::Lerp(
            kCoreRadiusMin,
            kCoreRadiusMax,
            avgIntensity
        );

        const float smoothingFactor = Helpers::Math::Clamp(
            kCoreRadiusSmoothing * deltaTime * 60.0f,
            0.0f,
            1.0f
        );

        m_currentCoreRadius = Helpers::Math::Lerp(
            m_currentCoreRadius,
            m_targetCoreRadius,
            smoothingFactor
        );
    }

    float PolylineWaveRenderer::CalculateAverageIntensity(
        const SpectrumData& spectrum
    ) const
    {
        if (spectrum.empty()) return 0.0f;

        float sum = 0.0f;
        for (float magnitude : spectrum) {
            sum += magnitude;
        }

        return Saturate(sum / spectrum.size());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::RenderOuterGlow(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float baseRadius,
        float barWidth
    ) const
    {
        const Color glowColor = m_primaryColor.WithAlpha(
            m_settings.lineWidth * kGlowAlpha
        );

        const Paint paint = Paint::Stroke(
            glowColor,
            barWidth * kOuterGlowStrokeMultiplier
        ).WithStrokeCap(StrokeCap::Round);

        auto drawGlow = [&]() {
            for (size_t i = 0; i < spectrum.size(); ++i) {
                if (!ShouldRenderGlow(spectrum[i])) continue;

                const float barLength = CalculateBarLength(spectrum[i], baseRadius);

                const Point start = Add(center, Multiply(m_barDirections[i], baseRadius));
                const Point end = Add(center, Multiply(m_barDirections[i], baseRadius + barLength));

                canvas.DrawLine(start, end, paint);
            }
            };

        canvas.DrawWithShadow(
            drawGlow,
            { 0.0f, 0.0f },
            kGlowBlurRadius,
            glowColor
        );
    }

    void PolylineWaveRenderer::RenderPulsingCore(
        Canvas& canvas,
        const Point& center,
        float baseRadius
    ) const
    {
        const float coreRadius = baseRadius * m_currentCoreRadius;

        const Color fillColor = m_primaryColor.WithAlpha(0.3f);
        canvas.DrawCircle(center, coreRadius, Paint::Fill(fillColor));

        const Color glowColor = m_primaryColor.WithAlpha(kCoreGlowAlpha);
        auto drawCoreGlow = [&]() {
            canvas.DrawCircle(
                center,
                coreRadius,
                Paint::Stroke(m_primaryColor, kCoreStrokeWidth)
            );
            };

        canvas.DrawWithShadow(
            drawCoreGlow,
            { 0.0f, 0.0f },
            kGlowBlurRadius * 0.5f,
            glowColor
        );

        drawCoreGlow();
    }

    void PolylineWaveRenderer::RenderGradientBars(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float baseRadius,
        float barWidth
    ) const
    {
        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (!ShouldRenderBar(spectrum[i])) continue;

            const float barLength = CalculateBarLength(spectrum[i], baseRadius);

            RenderGradientBar(
                canvas,
                center,
                m_barDirections[i],
                baseRadius,
                barLength,
                barWidth,
                spectrum[i]
            );
        }
    }

    void PolylineWaveRenderer::RenderSolidBars(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float baseRadius,
        float barWidth
    ) const
    {
        const Paint paint = Paint::Stroke(
            m_primaryColor,
            barWidth
        ).WithStrokeCap(StrokeCap::Round);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (!ShouldRenderBar(spectrum[i])) continue;

            const float barLength = CalculateBarLength(spectrum[i], baseRadius);

            const Point start = Add(center, Multiply(m_barDirections[i], baseRadius));
            const Point end = Add(center, Multiply(m_barDirections[i], baseRadius + barLength));

            canvas.DrawLine(start, end, paint);
        }
    }

    void PolylineWaveRenderer::RenderDynamicHighlights(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float baseRadius,
        float barWidth
    ) const
    {
        const Color highlightColor = Color::White().WithAlpha(
            m_settings.smoothness * kHighlightAlpha
        );

        const Paint paint = Paint::Stroke(
            highlightColor,
            barWidth * 0.4f
        ).WithStrokeCap(StrokeCap::Round);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (!ShouldRenderHighlight(spectrum[i])) continue;

            const float barLength = CalculateBarLength(spectrum[i], baseRadius);

            const float highlightStart = Helpers::Math::Lerp(
                baseRadius,
                baseRadius + barLength,
                kHighlightStartPosition
            );
            const float highlightEnd = baseRadius + barLength;

            const Point start = Add(center, Multiply(m_barDirections[i], highlightStart));
            const Point end = Add(center, Multiply(m_barDirections[i], highlightEnd));

            canvas.DrawLine(start, end, paint);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Individual Bar Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::RenderGradientBar(
        Canvas& canvas,
        const Point& center,
        const Point& direction,
        float baseRadius,
        float barLength,
        float barWidth,
        float magnitude
    ) const
    {
        const float segmentLength = barLength / kGradientSegments;
        const Paint paint = Paint::Stroke(
            Color::White(),
            barWidth
        ).WithStrokeCap(StrokeCap::Round);

        for (int seg = 0; seg < kGradientSegments; ++seg) {
            const float startDist = baseRadius + seg * segmentLength;
            const float endDist = baseRadius + (seg + 1) * segmentLength;

            const float normalizedPos = Helpers::Math::Normalize(
                static_cast<float>(seg),
                0.0f,
                static_cast<float>(kGradientSegments - 1)
            );

            const Color segmentColor = CalculateBarColorAtPosition(normalizedPos, magnitude);

            const Point start = Add(center, Multiply(direction, startDist));
            const Point end = Add(center, Multiply(direction, endDist));

            canvas.DrawLine(
                start,
                end,
                Paint::Stroke(segmentColor, barWidth).WithStrokeCap(StrokeCap::Round)
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float PolylineWaveRenderer::CalculateBaseRadius() const
    {
        return std::min(m_width, m_height) * 0.5f * kRadiusFactor;
    }

    float PolylineWaveRenderer::CalculateBarWidth(
        size_t barCount,
        float radius
    ) const
    {
        if (barCount == 0) return kMinBarWidth;

        const float circumference = TWO_PI * radius;
        const float idealWidth = circumference / barCount * kBarSpacingRatio;

        return Helpers::Math::Clamp(idealWidth, kMinBarWidth, kMaxBarWidth);
    }

    float PolylineWaveRenderer::CalculateBarLength(
        float magnitude,
        float radius
    ) const
    {
        const float normalizedLength = magnitude * radius * kBarLengthScale;
        const float minLength = radius * kMinBarLengthFactor;

        return std::max(normalizedLength, minLength);
    }

    Color PolylineWaveRenderer::CalculateBarColorAtPosition(
        float normalizedPosition,
        float magnitude
    ) const
    {
        const Color accentColor = GetAccentColor();

        const Color blended = Helpers::Color::InterpolateColor(
            m_primaryColor,
            accentColor,
            normalizedPosition
        );

        const float alpha = Helpers::Math::Lerp(0.8f, 1.0f, magnitude);
        return blended.WithAlpha(alpha);
    }

    Color PolylineWaveRenderer::GetAccentColor() const
    {
        const float h = m_primaryColor.r * 0.299f +
            m_primaryColor.g * 0.587f +
            m_primaryColor.b * 0.114f;

        if (h > 0.5f) {
            return Helpers::Color::AdjustBrightness(m_primaryColor, 1.5f);
        }
        else {
            return Color::White();
        }
    }

    Point PolylineWaveRenderer::GetViewportCenter() const
    {
        return Helpers::Geometry::GetViewportCenter(m_width, m_height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool PolylineWaveRenderer::ShouldRenderBar(float magnitude) const
    {
        return magnitude >= kMinMagnitude;
    }

    bool PolylineWaveRenderer::ShouldRenderGlow(float magnitude) const
    {
        return magnitude >= kGlowMagnitudeThreshold;
    }

    bool PolylineWaveRenderer::ShouldRenderHighlight(float magnitude) const
    {
        return magnitude >= kHighlightMagnitudeThreshold;
    }

} // namespace Spectrum