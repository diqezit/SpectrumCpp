// KenwoodBarsRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the KenwoodBarsRenderer for Kenwood-style bar visualization.
//
// Implementation details:
// - Fixed gradient palette recreates authentic Kenwood look
// - Peak hold algorithm: peaks stick at maximum for configurable time
// - Rendering layers ensure proper visual hierarchy
// - Overlay mode adjusts transparency and geometry for overlaid display
// - Gradient intensity boosted for vibrant appearance
//
// Rendering order:
// 1. Main gradient bars
// 2. White outlines (quality-dependent)
// 3. Peak indicators (white blocks)
// 4. Peak enhancement lines (quality-dependent)
//
// Performance optimizations:
// - Skip bars below visibility threshold
// - Batch rendering for peaks when possible
// - Pre-calculate gradient stops once per frame
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "KenwoodBarsRenderer.h"
#include "../API/D2DHelpers.h"
#include "../Base/RenderUtils.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "../API/Canvas.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kPeakFallSpeed = 0.25f;
        constexpr float kPeakHeight = 3.0f;
        constexpr float kPeakHeightOverlay = 2.0f;
        constexpr float kPeakHoldTimeS = 0.3f;

        constexpr float kMinBarHeight = 2.0f;
        constexpr float kMinMagnitudeForRender = 0.01f;
        constexpr float kCornerRadiusRatio = 0.25f;
        constexpr float kCornerRadiusRatioOverlay = 0.2f;

        constexpr float kOutlineWidth = 1.5f;
        constexpr float kOutlineWidthOverlay = 1.0f;
        constexpr float kOutlineAlpha = 0.5f;
        constexpr float kOutlineAlphaOverlay = 0.35f;
        constexpr float kPeakOutlineAlpha = 0.7f;
        constexpr float kPeakOutlineAlphaOverlay = 0.5f;

        constexpr float kGradientIntensityBoost = 1.1f;
        constexpr float kGradientIntensityBoostOverlay = 0.95f;

        constexpr float kOutlineMagnitudeScale = 1.5f;
        constexpr float kPeakCornerRadiusScale = 0.5f;
        constexpr float kPeakOutlineWidthScale = 0.75f;

        const std::vector<D2D1_GRADIENT_STOP> kBarGradientStopsBase = {
            { 0.00f, D2D1::ColorF(255 / 255.0f, 35 / 255.0f, 0.0f) },
            { 0.15f, D2D1::ColorF(255 / 255.0f, 85 / 255.0f, 0.0f) },
            { 0.15f, D2D1::ColorF(255 / 255.0f, 185 / 255.0f, 0.0f) },
            { 0.30f, D2D1::ColorF(255 / 255.0f, 235 / 255.0f, 0.0f) },
            { 0.30f, D2D1::ColorF(0.0f, 255 / 255.0f, 0 / 255.0f) },
            { 1.00f, D2D1::ColorF(0.0f, 240 / 255.0f, 120 / 255.0f) }
        };

        const Color kPeakColor = Color::White();
        const Color kPeakOutlineColor = Color(1.0f, 1.0f, 1.0f, 0.8f);
        const Color kSolidBarColor = Color::FromRGB(0, 240, 120);

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    KenwoodBarsRenderer::KenwoodBarsRenderer()
    {
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_currentSettings = { false, false, false, false };
            break;
        case RenderQuality::High:
            m_currentSettings = { true, true, true, true };
            break;
        case RenderQuality::Medium:
        default:
            m_currentSettings = { true, true, true, false };
            break;
        }
    }

    void KenwoodBarsRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        EnsurePeakArraySize(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            UpdatePeak(i, spectrum[i], deltaTime);
        }
    }

    void KenwoodBarsRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        const auto layout = RenderUtils::ComputeBarLayout(
            spectrum.size(),
            2.0f,
            m_width
        );

        if (layout.barWidth <= 0.0f) return;

        const float cornerRadius = CalculateCornerRadius(layout.barWidth);
        const auto barStyle = CreateBarStyle(cornerRadius);

        RenderMainLayer(canvas, spectrum, layout, barStyle);

        if (m_currentSettings.useOutline) {
            RenderOutlineLayer(canvas, spectrum, layout, cornerRadius);
        }

        RenderPeakLayer(canvas, layout, cornerRadius);

        if (m_currentSettings.useEnhancedPeaks) {
            RenderPeakEnhancementLayer(canvas, layout);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Peak Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::EnsurePeakArraySize(size_t size)
    {
        if (m_peaks.size() != size) {
            m_peaks.assign(size, 0.0f);
            m_peakTimers.assign(size, 0.0f);
        }
    }

    void KenwoodBarsRenderer::UpdatePeak(
        size_t index,
        float value,
        float deltaTime
    )
    {
        if (index >= m_peaks.size()) return;

        const float sanitizedValue = NormalizedFloat(value);

        if (sanitizedValue >= m_peaks[index]) {
            m_peaks[index] = sanitizedValue;
            m_peakTimers[index] = kPeakHoldTimeS;
        }
        else if (m_peakTimers[index] > 0.0f) {
            m_peakTimers[index] -= deltaTime;
        }
        else {
            m_peaks[index] = std::max(
                0.0f,
                m_peaks[index] - kPeakFallSpeed * deltaTime
            );
        }
    }

    float KenwoodBarsRenderer::GetPeakValue(size_t index) const
    {
        return (index < m_peaks.size()) ? m_peaks[index] : 0.0f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Style Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float KenwoodBarsRenderer::CalculateCornerRadius(float barWidth) const
    {
        if (!m_currentSettings.useRoundCorners) return 0.0f;

        const float ratio = m_isOverlay
            ? kCornerRadiusRatioOverlay
            : kCornerRadiusRatio;

        return barWidth * ratio;
    }

    BarStyle KenwoodBarsRenderer::CreateBarStyle(float cornerRadius) const
    {
        BarStyle style;
        style.spacing = 2.0f;
        style.cornerRadius = cornerRadius;
        style.useGradient = m_currentSettings.useGradient;

        if (style.useGradient) {
            style.gradientStops = GetAdjustedGradientStops();
        }

        return style;
    }

    std::vector<D2D1_GRADIENT_STOP> KenwoodBarsRenderer::GetAdjustedGradientStops() const
    {
        const float intensityBoost = m_isOverlay
            ? kGradientIntensityBoostOverlay
            : kGradientIntensityBoost;

        std::vector<D2D1_GRADIENT_STOP> adjustedStops;
        adjustedStops.reserve(kBarGradientStopsBase.size());

        for (const auto& stop : kBarGradientStopsBase) {
            D2D1_GRADIENT_STOP newStop = stop;
            newStop.color.r = std::min(1.0f, stop.color.r * intensityBoost);
            newStop.color.g = std::min(1.0f, stop.color.g * intensityBoost);
            newStop.color.b = std::min(1.0f, stop.color.b * intensityBoost);
            adjustedStops.push_back(newStop);
        }

        return adjustedStops;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::RenderMainLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const RenderUtils::BarLayout& /*layout*/,
        const BarStyle& barStyle
    ) const
    {
        if (spectrum.empty()) return;

        const Rect bounds{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height) };

        canvas.DrawSpectrumBars(spectrum, bounds, barStyle, kSolidBarColor);
    }

    void KenwoodBarsRenderer::RenderOutlineLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const RenderUtils::BarLayout& layout,
        float cornerRadius
    ) const
    {
        const float outlineWidth = m_isOverlay
            ? kOutlineWidthOverlay
            : kOutlineWidth;

        const float baseAlpha = m_isOverlay
            ? kOutlineAlphaOverlay
            : kOutlineAlpha;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);

            if (magnitude <= kMinMagnitudeForRender) continue;

            const float barHeight = std::max(
                RenderUtils::MagnitudeToHeight(magnitude, m_height),
                kMinBarHeight
            );

            const Rect barRect{
                i * layout.totalBarWidth,
                m_height - barHeight,
                layout.barWidth,
                barHeight
            };

            const float alpha = Utils::Saturate(magnitude * kOutlineMagnitudeScale) * baseAlpha;
            const Color outlineColor{ 1.0f, 1.0f, 1.0f, alpha };

            canvas.DrawRoundedRectangle(
                barRect,
                cornerRadius,
                Paint{ outlineColor, outlineWidth, false }
            );
        }
    }

    void KenwoodBarsRenderer::RenderPeakLayer(
        Canvas& canvas,
        const RenderUtils::BarLayout& layout,
        float cornerRadius
    ) const
    {
        const float peakHeight = m_isOverlay
            ? kPeakHeightOverlay
            : kPeakHeight;

        const float peakCornerRadius = cornerRadius * kPeakCornerRadiusScale;

        std::vector<Rect> peakRects;
        peakRects.reserve(m_peaks.size());

        for (size_t i = 0; i < m_peaks.size(); ++i) {
            const float peakValue = GetPeakValue(i);

            if (peakValue <= kMinMagnitudeForRender) continue;

            const float peakY = m_height - (peakValue * m_height);
            const Rect peakRect{
                i * layout.totalBarWidth,
                std::max(0.0f, peakY - peakHeight),
                layout.barWidth,
                peakHeight
            };

            peakRects.push_back(peakRect);
        }

        if (peakRects.empty()) return;

        if (peakCornerRadius > 0.0f) {
            for (const auto& rect : peakRects) {
                canvas.DrawRoundedRectangle(rect, peakCornerRadius, Paint{ kPeakColor, true });
            }
        }
        else {
            canvas.DrawRectangleBatch(peakRects, Paint{ kPeakColor, true });
        }
    }

    void KenwoodBarsRenderer::RenderPeakEnhancementLayer(
        Canvas& canvas,
        const RenderUtils::BarLayout& /*layout*/
    ) const
    {
        const float peakHeight = m_isOverlay
            ? kPeakHeightOverlay
            : kPeakHeight;

        const float outlineWidth = (m_isOverlay
            ? kOutlineWidthOverlay
            : kOutlineWidth) * kPeakOutlineWidthScale;

        const float baseAlpha = m_isOverlay
            ? kPeakOutlineAlphaOverlay
            : kPeakOutlineAlpha;

        Color outlineColor = kPeakOutlineColor;
        outlineColor.a = baseAlpha;

        if (m_peaks.empty()) return;

        const float totalBarWidth = static_cast<float>(m_width) / m_peaks.size();
        const float barWidth = totalBarWidth - 2.0f;

        for (size_t i = 0; i < m_peaks.size(); ++i) {
            const float peakValue = GetPeakValue(i);

            if (peakValue <= kMinMagnitudeForRender) continue;

            const float peakY = m_height - (peakValue * m_height);
            const Rect peakRect{
                i * totalBarWidth,
                std::max(0.0f, peakY - peakHeight),
                barWidth,
                peakHeight
            };

            canvas.DrawLine(
                { peakRect.x, peakRect.y },
                { peakRect.GetRight(), peakRect.y },
                outlineColor,
                outlineWidth
            );

            canvas.DrawLine(
                { peakRect.x, peakRect.GetBottom() },
                { peakRect.GetRight(), peakRect.GetBottom() },
                outlineColor,
                outlineWidth
            );
        }
    }

} // namespace Spectrum