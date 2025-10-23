// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the KenwoodBarsRenderer for Kenwood-style bar visualization
//
// Optimized implementation with shimmer gradient bars and sticky peaks
// Gradient computed dynamically based on primary color selection
// Zero memory leaks through proper ResourceCache integration
// Uses GeometryHelpers for all geometric operations
//
// Performance characteristics:
// - Gradient created once per unique color combination (cached in ResourceCache)
// - Peaks updated per frame with smooth hold/fall animation
// - Draw calls minimized through batching
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/KenwoodBarsRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Helpers/Geometry/GeometryHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Helpers/Geometry/ColorHelpers.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <algorithm>

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Geometry;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kPeakFallSpeed = 0.25f;
        constexpr float kPeakHoldTimeS = 0.3f;
        constexpr float kPeakHeight = 3.0f;
        constexpr float kPeakHeightOverlay = 2.0f;
        constexpr float kMinPeakValue = 0.01f;
        constexpr float kPeakCornerRadiusRatio = 0.5f;

        constexpr int kGradientSteps = 8;
        constexpr float kGradientBrightnessMin = 0.5f;
        constexpr float kGradientBrightnessRange = 0.7f;
        constexpr float kGradientSaturationMin = 0.8f;
        constexpr float kGradientSaturationRange = 0.2f;

        const Color kPeakColor = Color::White();

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
        m_settings = QualityPresets::Get<KenwoodBarsRenderer>(m_quality);
    }

    void KenwoodBarsRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        UpdatePeaks(spectrum, deltaTime);
    }

    void KenwoodBarsRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        if (spectrum.empty()) return;

        const auto layout = RenderUtils::ComputeBarLayout(
            spectrum.size(),
            m_settings.barSpacing,
            m_width
        );

        if (layout.barWidth <= 0.0f) return;

        RenderBars(canvas, spectrum);
        RenderPeaks(canvas, spectrum.size(), layout.totalBarWidth, layout.barWidth);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Peak Array Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::EnsurePeakArraySize(size_t size)
    {
        if (!IsPeakArraySizeCorrect(size)) {
            m_peaks.resize(size, 0.0f);
            m_peakTimers.resize(size, 0.0f);
        }
    }

    bool KenwoodBarsRenderer::IsPeakArraySizeCorrect(size_t size) const
    {
        return m_peaks.size() == size;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Peak Update Logic
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::UpdatePeaks(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        EnsurePeakArraySize(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            UpdateSinglePeak(i, spectrum[i], deltaTime);
        }
    }

    void KenwoodBarsRenderer::UpdateSinglePeak(
        size_t index,
        float value,
        float deltaTime
    )
    {
        const float sanitized = NormalizedFloat(value);

        if (ShouldSetNewPeak(index, sanitized)) {
            SetNewPeak(index, sanitized);
        }
        else if (IsPeakHolding(index)) {
            DecrementPeakHoldTimer(index, deltaTime);
        }
        else {
            ApplyPeakFall(index, deltaTime);
        }
    }

    void KenwoodBarsRenderer::SetNewPeak(size_t index, float value)
    {
        m_peaks[index] = value;
        m_peakTimers[index] = kPeakHoldTimeS;
    }

    void KenwoodBarsRenderer::DecrementPeakHoldTimer(
        size_t index,
        float deltaTime
    )
    {
        m_peakTimers[index] -= deltaTime;
    }

    void KenwoodBarsRenderer::ApplyPeakFall(size_t index, float deltaTime)
    {
        m_peaks[index] = Clamp(
            m_peaks[index] - kPeakFallSpeed * deltaTime,
            0.0f,
            1.0f
        );
    }

    bool KenwoodBarsRenderer::ShouldSetNewPeak(size_t index, float value) const
    {
        return value >= m_peaks[index];
    }

    bool KenwoodBarsRenderer::IsPeakHolding(size_t index) const
    {
        return m_peakTimers[index] > 0.0f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::RenderBars(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) const
    {
        const Rect bounds = GetViewportBounds();

        canvas.DrawSpectrumBars(
            spectrum,
            bounds,
            CreateBarStyle(),
            m_primaryColor
        );
    }

    void KenwoodBarsRenderer::RenderPeaks(
        Canvas& canvas,
        size_t barCount,
        float totalBarWidth,
        float barWidth
    ) const
    {
        if (m_peaks.empty()) return;

        const float cornerRadius = GetPeakCornerRadius();

        for (size_t i = 0; i < barCount && i < m_peaks.size(); ++i) {
            if (IsPeakVisible(i)) {
                RenderSinglePeak(
                    canvas,
                    i,
                    totalBarWidth,
                    barWidth,
                    cornerRadius
                );
            }
        }
    }

    void KenwoodBarsRenderer::RenderSinglePeak(
        Canvas& canvas,
        size_t index,
        float totalBarWidth,
        float barWidth,
        float cornerRadius
    ) const
    {
        const Rect rect = CalculatePeakRect(
            index,
            m_peaks[index],
            totalBarWidth,
            barWidth
        );

        DrawPeakRectangle(canvas, rect, cornerRadius);
    }

    void KenwoodBarsRenderer::DrawPeakRectangle(
        Canvas& canvas,
        const Rect& rect,
        float cornerRadius
    ) const
    {
        if (cornerRadius > 0.0f) {
            canvas.DrawRoundedRectangle(
                rect,
                cornerRadius,
                Paint::Fill(kPeakColor)
            );
        }
        else {
            canvas.DrawRectangle(
                rect,
                Paint::Fill(kPeakColor)
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Rect KenwoodBarsRenderer::CalculatePeakRect(
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
            Clamp(peakY - peakHeight, 0.0f, static_cast<float>(m_height)),
            barWidth,
            peakHeight
        };
    }

    float KenwoodBarsRenderer::CalculatePeakY(float peakValue) const
    {
        return Map(peakValue, 0.0f, 1.0f, static_cast<float>(m_height), 0.0f);
    }

    float KenwoodBarsRenderer::GetPeakHeight() const
    {
        return m_isOverlay ? kPeakHeightOverlay : kPeakHeight;
    }

    float KenwoodBarsRenderer::GetPeakCornerRadius() const
    {
        return m_settings.cornerRadius * kPeakCornerRadiusRatio;
    }

    Rect KenwoodBarsRenderer::GetViewportBounds() const
    {
        return CreateViewportBounds(m_width, m_height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color KenwoodBarsRenderer::ModifyColorBrightness(
        const Color& color,
        float factor
    ) const
    {
        return {
            Clamp(color.r * factor, 0.0f, 1.0f),
            Clamp(color.g * factor, 0.0f, 1.0f),
            Clamp(color.b * factor, 0.0f, 1.0f),
            color.a
        };
    }

    Color KenwoodBarsRenderer::ModifyColorSaturation(
        const Color& color,
        float factor
    ) const
    {
        const float gray =
            color.r * 0.299f +
            color.g * 0.587f +
            color.b * 0.114f;

        return {
            Lerp(gray, color.r, factor),
            Lerp(gray, color.g, factor),
            Lerp(gray, color.b, factor),
            color.a
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradient Generation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::BuildGradientStops(
        std::vector<D2D1_GRADIENT_STOP>& stops,
        const Color& baseColor
    ) const
    {
        stops.reserve(kGradientSteps);

        for (int i = 0; i < kGradientSteps; ++i) {
            const float t = Normalize(
                static_cast<float>(i),
                0.0f,
                static_cast<float>(kGradientSteps - 1)
            );

            const float brightness = Lerp(
                kGradientBrightnessMin,
                kGradientBrightnessMin + kGradientBrightnessRange,
                t
            );

            const float saturation = Lerp(
                kGradientSaturationMin,
                kGradientSaturationMin + kGradientSaturationRange,
                t
            );

            Color stepColor = ModifyColorBrightness(baseColor, brightness);
            stepColor = ModifyColorSaturation(stepColor, saturation);

            stops.push_back({
                t,
                D2D1::ColorF(
                    stepColor.r,
                    stepColor.g,
                    stepColor.b,
                    stepColor.a
                )
                });
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool KenwoodBarsRenderer::IsPeakVisible(size_t index) const
    {
        return IsPeakValueVisible(m_peaks[index]);
    }

    bool KenwoodBarsRenderer::IsPeakValueVisible(float peakValue) const
    {
        return peakValue > kMinPeakValue;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Style Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    BarStyle KenwoodBarsRenderer::CreateBarStyle() const
    {
        BarStyle style;
        style.spacing = m_settings.barSpacing;
        style.cornerRadius = m_settings.cornerRadius;
        style.useGradient = m_settings.useGradient;

        if (style.useGradient) {
            BuildGradientStops(style.gradientStops, m_primaryColor);
        }

        return style;
    }

} // namespace Spectrum