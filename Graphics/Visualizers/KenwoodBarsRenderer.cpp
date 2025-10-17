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

#include "Graphics/Visualizers/KenwoodBarsRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include <algorithm>

namespace Spectrum {

    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kPeakFallSpeed = 0.25f;
        constexpr float kPeakHoldTimeS = 0.3f;

        constexpr float kPeakHeight = 3.0f;
        constexpr float kPeakHeightOverlay = 2.0f;

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

        constexpr float kBarSpacing = 2.0f;

        const std::vector<D2D1_GRADIENT_STOP> kBarGradientStopsBase = {
            { 0.00f, D2D1::ColorF(255 / 255.0f, 35 / 255.0f, 0.0f) },
            { 0.15f, D2D1::ColorF(255 / 255.0f, 85 / 255.0f, 0.0f) },
            { 0.15f, D2D1::ColorF(255 / 255.0f, 185 / 255.0f, 0.0f) },
            { 0.30f, D2D1::ColorF(255 / 255.0f, 235 / 255.0f, 0.0f) },
            { 0.30f, D2D1::ColorF(0.0f, 255 / 255.0f, 0 / 255.0f) },
            { 1.00f, D2D1::ColorF(0.0f, 240 / 255.0f, 120 / 255.0f) }
        };

        const Color kPeakColor = Color::White();
        const Color kPeakOutlineColorBase = Color(1.0f, 1.0f, 1.0f, 0.8f);
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
            GetBarSpacing(),
            m_width
        );

        if (layout.barWidth <= 0.0f) return;

        const float cornerRadius = CalculateCornerRadius(layout.barWidth);
        const BarStyle barStyle = CreateBarStyle(cornerRadius);

        RenderMainLayer(canvas, spectrum, barStyle);

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
        if (!IsPeakIndexValid(index)) return;

        const float sanitizedValue = NormalizedFloat(value);

        if (ShouldUpdatePeak(sanitizedValue, index)) {
            m_peaks[index] = sanitizedValue;
            m_peakTimers[index] = kPeakHoldTimeS;
        }
        else if (IsPeakHoldActive(index)) {
            UpdatePeakHoldTimer(index, deltaTime);
        }
        else {
            UpdatePeakFall(index, deltaTime);
        }
    }

    void KenwoodBarsRenderer::UpdatePeakHoldTimer(
        size_t index,
        float deltaTime
    )
    {
        m_peakTimers[index] -= deltaTime;
    }

    void KenwoodBarsRenderer::UpdatePeakFall(
        size_t index,
        float deltaTime
    )
    {
        m_peaks[index] = std::max(
            0.0f,
            m_peaks[index] - kPeakFallSpeed * deltaTime
        );
    }

    float KenwoodBarsRenderer::GetPeakValue(size_t index) const
    {
        return IsPeakIndexValid(index) ? m_peaks[index] : 0.0f;
    }

    bool KenwoodBarsRenderer::IsPeakIndexValid(size_t index) const
    {
        return index < m_peaks.size();
    }

    bool KenwoodBarsRenderer::ShouldUpdatePeak(
        float value,
        size_t index
    ) const
    {
        return value >= m_peaks[index];
    }

    bool KenwoodBarsRenderer::IsPeakHoldActive(size_t index) const
    {
        return m_peakTimers[index] > 0.0f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Style Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float KenwoodBarsRenderer::CalculateCornerRadius(float barWidth) const
    {
        if (!m_currentSettings.useRoundCorners) return 0.0f;

        return barWidth * GetCornerRadiusRatio();
    }

    float KenwoodBarsRenderer::GetCornerRadiusRatio() const
    {
        return m_isOverlay ? kCornerRadiusRatioOverlay : kCornerRadiusRatio;
    }

    BarStyle KenwoodBarsRenderer::CreateBarStyle(float cornerRadius) const
    {
        BarStyle style;
        style.spacing = GetBarSpacing();
        style.cornerRadius = cornerRadius;
        style.useGradient = m_currentSettings.useGradient;

        if (style.useGradient) {
            style.gradientStops = GetAdjustedGradientStops();
        }

        return style;
    }

    float KenwoodBarsRenderer::GetBarSpacing() const
    {
        return kBarSpacing;
    }

    std::vector<D2D1_GRADIENT_STOP> KenwoodBarsRenderer::GetAdjustedGradientStops() const
    {
        const float intensityBoost = GetGradientIntensityBoost();

        std::vector<D2D1_GRADIENT_STOP> adjustedStops;
        adjustedStops.reserve(kBarGradientStopsBase.size());

        for (const auto& stop : kBarGradientStopsBase) {
            adjustedStops.push_back(AdjustGradientStop(stop, intensityBoost));
        }

        return adjustedStops;
    }

    float KenwoodBarsRenderer::GetGradientIntensityBoost() const
    {
        return m_isOverlay ? kGradientIntensityBoostOverlay : kGradientIntensityBoost;
    }

    D2D1_GRADIENT_STOP KenwoodBarsRenderer::AdjustGradientStop(
        const D2D1_GRADIENT_STOP& stop,
        float intensityBoost
    ) const
    {
        D2D1_GRADIENT_STOP newStop = stop;
        newStop.color.r = std::min(1.0f, stop.color.r * intensityBoost);
        newStop.color.g = std::min(1.0f, stop.color.g * intensityBoost);
        newStop.color.b = std::min(1.0f, stop.color.b * intensityBoost);
        return newStop;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::RenderMainLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const BarStyle& barStyle
    ) const
    {
        if (spectrum.empty()) return;

        const Rect bounds{
            0.0f,
            0.0f,
            static_cast<float>(m_width),
            static_cast<float>(m_height)
        };

        canvas.DrawSpectrumBars(spectrum, bounds, barStyle, kSolidBarColor);
    }

    void KenwoodBarsRenderer::RenderOutlineLayer(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const RenderUtils::BarLayout& layout,
        float cornerRadius
    ) const
    {
        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);

            if (!ShouldRenderBar(magnitude)) continue;

            const Rect barRect = CalculateBarRect(i, magnitude, layout);

            RenderBarOutline(canvas, barRect, magnitude, cornerRadius);
        }
    }

    void KenwoodBarsRenderer::RenderPeakLayer(
        Canvas& canvas,
        const RenderUtils::BarLayout& layout,
        float cornerRadius
    ) const
    {
        std::vector<Rect> peakRects;
        peakRects.reserve(m_peaks.size());

        CollectPeakRects(peakRects, layout);

        if (peakRects.empty()) return;

        const float peakCornerRadius = GetPeakCornerRadius(cornerRadius);

        RenderPeakRects(canvas, peakRects, peakCornerRadius);
    }

    void KenwoodBarsRenderer::RenderPeakEnhancementLayer(
        Canvas& canvas,
        const RenderUtils::BarLayout& /*layout*/
    ) const
    {
        if (m_peaks.empty()) return;

        const float totalBarWidth = static_cast<float>(m_width) / m_peaks.size();
        const float barWidth = totalBarWidth - GetBarSpacing();

        for (size_t i = 0; i < m_peaks.size(); ++i) {
            RenderPeakEnhancement(canvas, i, totalBarWidth, barWidth);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Bar Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::RenderBarOutline(
        Canvas& canvas,
        const Rect& barRect,
        float magnitude,
        float cornerRadius
    ) const
    {
        const Color outlineColor = GetOutlineColor(magnitude);
        const float outlineWidth = GetOutlineWidth();

        canvas.DrawRoundedRectangle(
            barRect,
            cornerRadius,
            Paint::Stroke(outlineColor, outlineWidth)
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Peak Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::CollectPeakRects(
        std::vector<Rect>& peakRects,
        const RenderUtils::BarLayout& layout
    ) const
    {
        for (size_t i = 0; i < m_peaks.size(); ++i) {
            const float peakValue = GetPeakValue(i);

            if (!ShouldRenderPeak(peakValue)) continue;

            peakRects.push_back(CalculatePeakRect(i, peakValue, layout));
        }
    }

    void KenwoodBarsRenderer::RenderPeakRects(
        Canvas& canvas,
        const std::vector<Rect>& peakRects,
        float cornerRadius
    ) const
    {
        if (cornerRadius > 0.0f) {
            for (const auto& rect : peakRects) {
                canvas.DrawRoundedRectangle(
                    rect,
                    cornerRadius,
                    Paint::Fill(kPeakColor)
                );
            }
        }
        else {
            canvas.DrawRectangleBatch(peakRects, Paint::Fill(kPeakColor));
        }
    }

    void KenwoodBarsRenderer::RenderPeakEnhancement(
        Canvas& canvas,
        size_t index,
        float totalBarWidth,
        float barWidth
    ) const
    {
        const float peakValue = GetPeakValue(index);

        if (!ShouldRenderPeak(peakValue)) return;

        const Rect peakRect = CalculatePeakRectForEnhancement(
            index,
            peakValue,
            totalBarWidth,
            barWidth
        );

        const Color outlineColor = GetPeakOutlineColor();
        const float outlineWidth = GetPeakEnhancementOutlineWidth();

        RenderPeakEnhancementLines(canvas, peakRect, outlineColor, outlineWidth);
    }

    void KenwoodBarsRenderer::RenderPeakEnhancementLines(
        Canvas& canvas,
        const Rect& peakRect,
        const Color& outlineColor,
        float outlineWidth
    ) const
    {
        canvas.DrawLine(
            { peakRect.x, peakRect.y },
            { peakRect.GetRight(), peakRect.y },
            Paint::Stroke(outlineColor, outlineWidth)
        );

        canvas.DrawLine(
            { peakRect.x, peakRect.GetBottom() },
            { peakRect.GetRight(), peakRect.GetBottom() },
            Paint::Stroke(outlineColor, outlineWidth)
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Rect KenwoodBarsRenderer::CalculateBarRect(
        size_t index,
        float magnitude,
        const RenderUtils::BarLayout& layout
    ) const
    {
        const float barHeight = CalculateBarHeight(magnitude);

        return {
            index * layout.totalBarWidth,
            m_height - barHeight,
            layout.barWidth,
            barHeight
        };
    }

    Rect KenwoodBarsRenderer::CalculatePeakRect(
        size_t index,
        float peakValue,
        const RenderUtils::BarLayout& layout
    ) const
    {
        const float peakY = CalculatePeakY(peakValue);
        const float peakHeight = GetPeakHeight();

        return {
            index * layout.totalBarWidth,
            std::max(0.0f, peakY - peakHeight),
            layout.barWidth,
            peakHeight
        };
    }

    Rect KenwoodBarsRenderer::CalculatePeakRectForEnhancement(
        size_t index,
        float peakValue,
        float totalBarWidth,
        float barWidth
    ) const
    {
        const float peakY = CalculatePeakY(peakValue);
        const float peakHeight = GetPeakHeight();

        return {
            index * totalBarWidth,
            std::max(0.0f, peakY - peakHeight),
            barWidth,
            peakHeight
        };
    }

    float KenwoodBarsRenderer::CalculateBarHeight(float magnitude) const
    {
        return std::max(
            RenderUtils::MagnitudeToHeight(magnitude, m_height),
            kMinBarHeight
        );
    }

    float KenwoodBarsRenderer::CalculatePeakY(float peakValue) const
    {
        return m_height - (peakValue * m_height);
    }

    float KenwoodBarsRenderer::GetPeakHeight() const
    {
        return m_isOverlay ? kPeakHeightOverlay : kPeakHeight;
    }

    float KenwoodBarsRenderer::GetPeakCornerRadius(float baseCornerRadius) const
    {
        return baseCornerRadius * kPeakCornerRadiusScale;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color KenwoodBarsRenderer::GetOutlineColor(float magnitude) const
    {
        const float alpha = Utils::Saturate(
            magnitude * kOutlineMagnitudeScale
        ) * GetOutlineAlpha();

        return Color(1.0f, 1.0f, 1.0f, alpha);
    }

    Color KenwoodBarsRenderer::GetPeakOutlineColor() const
    {
        return kPeakOutlineColorBase.WithAlpha(GetPeakOutlineAlpha());
    }

    float KenwoodBarsRenderer::GetOutlineAlpha() const
    {
        return m_isOverlay ? kOutlineAlphaOverlay : kOutlineAlpha;
    }

    float KenwoodBarsRenderer::GetPeakOutlineAlpha() const
    {
        return m_isOverlay ? kPeakOutlineAlphaOverlay : kPeakOutlineAlpha;
    }

    float KenwoodBarsRenderer::GetOutlineWidth() const
    {
        return m_isOverlay ? kOutlineWidthOverlay : kOutlineWidth;
    }

    float KenwoodBarsRenderer::GetPeakEnhancementOutlineWidth() const
    {
        return GetOutlineWidth() * kPeakOutlineWidthScale;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool KenwoodBarsRenderer::ShouldRenderBar(float magnitude) const
    {
        return magnitude > kMinMagnitudeForRender;
    }

    bool KenwoodBarsRenderer::ShouldRenderPeak(float peakValue) const
    {
        return peakValue > kMinMagnitudeForRender;
    }

    bool KenwoodBarsRenderer::IsBarHeightValid(float height) const
    {
        return height >= kMinBarHeight;
    }

} // namespace Spectrum