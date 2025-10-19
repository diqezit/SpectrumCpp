#include "KenwoodBarsRenderer.h"
#include "RenderUtils.h"
#include "MathUtils.h"
#include "ColorUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        // Peak behavior
        constexpr float PEAK_FALL_SPEED = 0.25f;
        constexpr float PEAK_HEIGHT = 3.0f;
        constexpr float PEAK_HEIGHT_OVERLAY = 2.0f;
        constexpr float PEAK_HOLD_TIME_S = 0.3f;

        // Bar geometry
        constexpr float MIN_BAR_HEIGHT = 2.0f;
        constexpr float MIN_MAGNITUDE_FOR_RENDER = 0.01f;
        constexpr float CORNER_RADIUS_RATIO = 0.25f;
        constexpr float CORNER_RADIUS_RATIO_OVERLAY = 0.2f;

        // Outline styles
        constexpr float OUTLINE_WIDTH = 1.5f;
        constexpr float OUTLINE_WIDTH_OVERLAY = 1.0f;
        constexpr float OUTLINE_ALPHA = 0.5f;
        constexpr float OUTLINE_ALPHA_OVERLAY = 0.35f;
        constexpr float PEAK_OUTLINE_ALPHA = 0.7f;
        constexpr float PEAK_OUTLINE_ALPHA_OVERLAY = 0.5f;

        // Gradient boost
        constexpr float GRADIENT_INTENSITY_BOOST = 1.1f;
        constexpr float GRADIENT_INTENSITY_BOOST_OVERLAY = 0.95f;

        const std::vector<D2D1_GRADIENT_STOP> BAR_GRADIENT_STOPS_BASE = {
            { 0.00f, D2D1::ColorF(255 / 255.f, 35 / 255.f, 0.f) },
            { 0.15f, D2D1::ColorF(255 / 255.f, 85 / 255.f, 0.f) },
            { 0.15f, D2D1::ColorF(255 / 255.f, 185 / 255.f, 0.f) },
            { 0.30f, D2D1::ColorF(255 / 255.f, 235 / 255.f, 0.f) },
            { 0.30f, D2D1::ColorF(0.f, 255 / 255.f, 0 / 255.f) },
            { 1.00f, D2D1::ColorF(0.f, 240 / 255.f, 120 / 255.f) }
        };

        const Color PEAK_COLOR = Color::White();
        const Color PEAK_OUTLINE_COLOR = Color(1.0f, 1.0f, 1.0f, 0.8f);
        const Color SOLID_BAR_COLOR = Color::FromRGB(0, 240, 120);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor & Settings
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    KenwoodBarsRenderer::KenwoodBarsRenderer() {
        UpdateSettings();
    }

    // user can select quality to trade visuals for performance
    // high quality enables all effects, low quality disables them
    void KenwoodBarsRenderer::UpdateSettings() {
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Core Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void KenwoodBarsRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        EnsurePeakArraySize(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            UpdatePeak(i, spectrum[i], deltaTime);
        }
    }

    // renders in layers for a sense of depth: main bars, then outline, then peaks
    void KenwoodBarsRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        auto layout = RenderUtils::ComputeBarLayout(
            spectrum.size(),
            2.0f,
            m_width
        );
        if (layout.barWidth <= 0) {
            return;
        }

        float cornerRadius = CalculateCornerRadius(layout.barWidth);
        auto barStyle = CreateBarStyle(cornerRadius);

        RenderMainLayer(context, spectrum, layout, barStyle);

        if (m_currentSettings.useOutline) {
            RenderOutlineLayer(context, spectrum, layout, cornerRadius);
        }

        RenderPeakLayer(context, layout, cornerRadius);

        if (m_currentSettings.useEnhancedPeaks) {
            RenderPeakEnhancementLayer(context, layout);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helper Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    // resize peak data arrays if spectrum size changes
    // this avoids reallocating on every frame
    void KenwoodBarsRenderer::EnsurePeakArraySize(size_t size) {
        if (m_peaks.size() != size) {
            m_peaks.assign(size, 0.0f);
            m_peakTimers.assign(size, 0.0f);
        }
    }

    // peak holds at its highest point for a moment then falls
    // this gives a classic, sticky peak effect
    void KenwoodBarsRenderer::UpdatePeak(
        size_t index,
        float value,
        float deltaTime
    ) {
        if (index >= m_peaks.size()) {
            return;
        }

        if (value >= m_peaks[index]) {
            m_peaks[index] = value;
            m_peakTimers[index] = PEAK_HOLD_TIME_S;
        }
        else if (m_peakTimers[index] > 0.0f) {
            m_peakTimers[index] -= deltaTime;
        }
        else {
            m_peaks[index] = std::max(
                0.0f,
                m_peaks[index] - PEAK_FALL_SPEED * deltaTime
            );
        }
    }

    float KenwoodBarsRenderer::GetPeakValue(size_t index) const {
        return (index < m_peaks.size()) ? m_peaks[index] : 0.0f;
    }

    // overlay mode uses smaller radius for a cleaner look when drawn over content
    float KenwoodBarsRenderer::CalculateCornerRadius(float barWidth) const {
        if (!m_currentSettings.useRoundCorners) {
            return 0.0f;
        }

        float ratio = m_isOverlay
            ? CORNER_RADIUS_RATIO_OVERLAY
            : CORNER_RADIUS_RATIO;

        return barWidth * ratio;
    }

    // assembles the BarStyle struct for the main bar rendering call
    BarStyle KenwoodBarsRenderer::CreateBarStyle(
        float cornerRadius
    ) const {
        BarStyle style;
        style.spacing = 2.0f;
        style.cornerRadius = cornerRadius;
        style.useGradient = m_currentSettings.useGradient;

        if (style.useGradient) {
            style.gradientStops = GetAdjustedGradientStops();
        }

        return style;
    }

    // Kenwood style has a very bright, saturated look
    // boost gradient colors to achieve this effect
    std::vector<D2D1_GRADIENT_STOP> KenwoodBarsRenderer::GetAdjustedGradientStops(
    ) const {
        float intensityBoost = m_isOverlay
            ? GRADIENT_INTENSITY_BOOST_OVERLAY
            : GRADIENT_INTENSITY_BOOST;

        std::vector<D2D1_GRADIENT_STOP> adjustedStops;
        adjustedStops.reserve(BAR_GRADIENT_STOPS_BASE.size());

        for (const auto& stop : BAR_GRADIENT_STOPS_BASE) {
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

    // delegates main bar drawing to the generic spectrum bar renderer
    void KenwoodBarsRenderer::RenderMainLayer(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const RenderUtils::BarLayout& layout,
        const BarStyle& barStyle
    ) const {
        if (spectrum.empty()) {
            return;
        }

        Rect bounds(0, 0, static_cast<float>(m_width), static_cast<float>(m_height));

        context.DrawSpectrumBars(
            spectrum,
            bounds,
            barStyle,
            SOLID_BAR_COLOR
        );
    }

    // draw a subtle outline around each bar for definition
    // outline alpha fades with bar height for a softer look
    void KenwoodBarsRenderer::RenderOutlineLayer(
        GraphicsContext& context,
        const SpectrumData& spectrum,
        const RenderUtils::BarLayout& layout,
        float cornerRadius
    ) const {
        float outlineWidth = m_isOverlay
            ? OUTLINE_WIDTH_OVERLAY
            : OUTLINE_WIDTH;
        float baseAlpha = m_isOverlay
            ? OUTLINE_ALPHA_OVERLAY
            : OUTLINE_ALPHA;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            float magnitude = spectrum[i];

            if (magnitude <= MIN_MAGNITUDE_FOR_RENDER) {
                continue;
            }

            float barHeight = std::max(
                RenderUtils::MagnitudeToHeight(magnitude, m_height),
                MIN_BAR_HEIGHT
            );

            Rect barRect(
                i * layout.totalBarWidth,
                m_height - barHeight,
                layout.barWidth,
                barHeight
            );

            float alpha = Utils::Saturate(magnitude * 1.5f) * baseAlpha;
            Color outlineColor(1.0f, 1.0f, 1.0f, alpha);

            context.DrawRoundedRectangle(
                barRect,
                cornerRadius,
                outlineColor,
                false,
                outlineWidth
            );
        }
    }

    // render peak indicators as solid blocks
    // batch rendering is used for performance when drawing many peaks
    void KenwoodBarsRenderer::RenderPeakLayer(
        GraphicsContext& context,
        const RenderUtils::BarLayout& layout,
        float cornerRadius
    ) const {
        float peakHeight = m_isOverlay
            ? PEAK_HEIGHT_OVERLAY
            : PEAK_HEIGHT;
        float peakCornerRadius = cornerRadius * 0.5f;

        std::vector<Rect> peakRects;
        peakRects.reserve(m_peaks.size());

        for (size_t i = 0; i < m_peaks.size(); ++i) {
            float peakValue = GetPeakValue(i);

            if (peakValue <= MIN_MAGNITUDE_FOR_RENDER) {
                continue;
            }

            float peakY = m_height - (peakValue * m_height);
            Rect peakRect(
                i * layout.totalBarWidth,
                std::max(0.0f, peakY - peakHeight),
                layout.barWidth,
                peakHeight
            );

            peakRects.push_back(peakRect);
        }

        if (peakRects.empty()) {
            return;
        }

        if (peakCornerRadius > 0.0f) {
            for (const auto& rect : peakRects) {
                context.DrawRoundedRectangle(
                    rect,
                    peakCornerRadius,
                    PEAK_COLOR,
                    true
                );
            }
        }
        else {
            context.DrawRectangleBatch(peakRects, PEAK_COLOR, true);
        }
    }

    // add horizontal lines to the peak indicators for a more technical look
    // this gives the peaks a distinct, blocky appearance
    void KenwoodBarsRenderer::RenderPeakEnhancementLayer(
        GraphicsContext& context,
        const RenderUtils::BarLayout& layout
    ) const {
        float peakHeight = m_isOverlay
            ? PEAK_HEIGHT_OVERLAY
            : PEAK_HEIGHT;
        float outlineWidth = (m_isOverlay
            ? OUTLINE_WIDTH_OVERLAY
            : OUTLINE_WIDTH) * 0.75f;
        float baseAlpha = m_isOverlay
            ? PEAK_OUTLINE_ALPHA_OVERLAY
            : PEAK_OUTLINE_ALPHA;

        Color outlineColor = PEAK_OUTLINE_COLOR;
        outlineColor.a = baseAlpha;

        for (size_t i = 0; i < m_peaks.size(); ++i) {
            float peakValue = GetPeakValue(i);

            if (peakValue <= MIN_MAGNITUDE_FOR_RENDER) {
                continue;
            }

            float peakY = m_height - (peakValue * m_height);
            Rect peakRect(
                i * layout.totalBarWidth,
                std::max(0.0f, peakY - peakHeight),
                layout.barWidth,
                peakHeight
            );

            context.DrawLine(
                { peakRect.x, peakRect.y },
                { peakRect.GetRight(), peakRect.y },
                outlineColor,
                outlineWidth
            );

            context.DrawLine(
                { peakRect.x, peakRect.GetBottom() },
                { peakRect.GetRight(), peakRect.GetBottom() },
                outlineColor,
                outlineWidth
            );
        }
    }

} // namespace Spectrum