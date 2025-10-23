// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the PolylineWaveRenderer for sunburst/starburst visualization.
//
// Implementation details:
// - Bars radiate from center with length based on frequency magnitude
// - Direction vectors pre-calculated once per bar count change
// - Multi-layer rendering for proper effect composition
// - Bar width dynamically adjusted to prevent overlap
// - Glow effect achieved through shadow with blur
//
// Rendering pipeline:
// 1. Inner circle: decorative ring at base of bars
// 2. Glow layer: blurred shadow behind high-intensity bars
// 3. Main bars: primary visualization with rounded caps
// 4. Highlight layer: white accents on peaks for emphasis
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/PolylineWaveRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include "Common/Types.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        // Radius calculations
        constexpr float kRadiusFactor = 0.8f;
        constexpr float kInnerRadiusFactor = 0.9f;

        // Bar dimensions
        constexpr float kMinBarLengthFactor = 0.03f;
        constexpr float kBarLengthScale = 0.5f;
        constexpr float kMinBarWidth = 2.0f;
        constexpr float kMaxBarWidth = 20.0f;

        // Magnitude thresholds
        constexpr float kMagnitudeThreshold = 0.01f;
        constexpr float kGlowMagnitudeThreshold = 0.6f;
        constexpr float kHighlightMagnitudeThreshold = 0.4f;

        // Effect parameters
        constexpr float kHighlightStartOffset = 0.7f;
        constexpr float kHighlightStrokeMultiplier = 0.5f;
        constexpr float kGlowStrokeMultiplier = 1.5f;
        constexpr float kInnerCircleStrokeWidth = 2.0f;
        constexpr float kGlowBlurRadius = 3.0f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    PolylineWaveRenderer::PolylineWaveRenderer()
    {
        m_primaryColor = Color::FromRGB(0, 180, 255);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = {
                false,  // useGlow
                false,  // useHighlight
                0.0f,   // glowIntensity
                0.3f,   // innerCircleAlpha
                0.7f    // barSpacingRatio
            };
            break;
        case RenderQuality::High:
            m_settings = {
                true,   // useGlow
                true,   // useHighlight
                0.6f,   // glowIntensity
                0.5f,   // innerCircleAlpha
                0.8f    // barSpacingRatio
            };
            break;
        case RenderQuality::Medium:
        default:
            m_settings = {
                true,   // useGlow
                false,  // useHighlight
                0.4f,   // glowIntensity
                0.4f,   // innerCircleAlpha
                0.75f   // barSpacingRatio
            };
            break;
        }
    }

    void PolylineWaveRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        if (spectrum.empty()) return;

        EnsureBarDirections(spectrum.size());

        const Point center = {
            m_width * 0.5f,
            m_height * 0.5f
        };

        const float radius = CalculateRadius();
        const float barWidth = CalculateBarWidth(spectrum.size(), radius);

        // Render layers in order for proper composition
        RenderInnerCircle(canvas, center, radius);

        if (m_settings.useGlow) {
            RenderGlowLayer(
                canvas,
                spectrum,
                center,
                radius,
                barWidth
            );
        }

        RenderMainBars(
            canvas,
            spectrum,
            center,
            radius,
            barWidth
        );

        if (m_settings.useHighlight) {
            RenderHighlightLayer(
                canvas,
                spectrum,
                center,
                radius,
                barWidth
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Rendering Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::RenderInnerCircle(
        Canvas& canvas,
        const Point& center,
        float radius
    ) const
    {
        const Color circleColor = m_primaryColor.WithAlpha(m_settings.innerCircleAlpha);
        const Paint paint = Paint::Stroke(circleColor, kInnerCircleStrokeWidth);

        canvas.DrawCircle(
            center,
            radius * kInnerRadiusFactor,
            paint
        );
    }

    void PolylineWaveRenderer::RenderGlowLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float radius,
        float barWidth
    ) const
    {
        const Color glowColor = m_primaryColor.WithAlpha(m_settings.glowIntensity);
        const Paint paint = Paint::Stroke(
            glowColor,
            barWidth * kGlowStrokeMultiplier
        ).WithStrokeCap(StrokeCap::Round);

        auto drawGlow = [&]() {
            for (size_t i = 0; i < spectrum.size(); ++i) {
                if (spectrum[i] < kGlowMagnitudeThreshold) continue;

                const float barLength = CalculateBarLength(spectrum[i], radius);
                const Point startPoint = center + m_barDirections[i] * radius;
                const Point endPoint = center + m_barDirections[i] * (radius + barLength);

                canvas.DrawLine(startPoint, endPoint, paint);
            }
            };

        canvas.DrawWithShadow(
            drawGlow,
            { 0.0f, 0.0f },
            kGlowBlurRadius,
            glowColor.WithAlpha(1.0f)
        );
    }

    void PolylineWaveRenderer::RenderMainBars(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float radius,
        float barWidth
    ) const
    {
        const Paint paint = Paint::Stroke(
            m_primaryColor,
            barWidth
        ).WithStrokeCap(StrokeCap::Round);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (spectrum[i] < kMagnitudeThreshold) continue;

            const float barLength = CalculateBarLength(spectrum[i], radius);
            const Point startPoint = center + m_barDirections[i] * radius;
            const Point endPoint = center + m_barDirections[i] * (radius + barLength);

            canvas.DrawLine(startPoint, endPoint, paint);
        }
    }

    void PolylineWaveRenderer::RenderHighlightLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float radius,
        float barWidth
    ) const
    {
        const Paint paint = Paint::Stroke(
            Color::White(),
            barWidth * kHighlightStrokeMultiplier
        ).WithStrokeCap(StrokeCap::Round);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (spectrum[i] < kHighlightMagnitudeThreshold) continue;

            const float barLength = CalculateBarLength(spectrum[i], radius);
            const float highlightStart = radius + barLength * kHighlightStartOffset;
            const float highlightEnd = radius + barLength;

            const Point startPoint = center + m_barDirections[i] * highlightStart;
            const Point endPoint = center + m_barDirections[i] * highlightEnd;

            canvas.DrawLine(startPoint, endPoint, paint);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PolylineWaveRenderer::EnsureBarDirections(size_t barCount)
    {
        if (m_barDirections.size() != barCount) {
            m_barDirections.resize(barCount);

            const float angleStep = TWO_PI / barCount;

            for (size_t i = 0; i < barCount; ++i) {
                const float angle = i * angleStep;
                m_barDirections[i] = {
                    std::cos(angle),
                    std::sin(angle)
                };
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float PolylineWaveRenderer::CalculateRadius() const
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
        const float idealWidth = circumference / barCount * m_settings.barSpacingRatio;

        return Utils::Clamp(
            idealWidth,
            kMinBarWidth,
            kMaxBarWidth
        );
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

} // namespace Spectrum