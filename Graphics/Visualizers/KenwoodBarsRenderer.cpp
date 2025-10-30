#include "Graphics/Visualizers/KenwoodBarsRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    namespace {
        constexpr float kPeakHoldTime = 0.3f;
        constexpr float kPeakDecayRate = 0.95f;
        constexpr float kPeakHeight = 3.0f;
        constexpr float kPeakHeightOverlay = 2.0f;
        constexpr int kGradientSteps = 8;
    }

    KenwoodBarsRenderer::KenwoodBarsRenderer() {
        InitializePeakTracker(0, kPeakHoldTime, kPeakDecayRate);
        UpdateSettings();
    }

    void KenwoodBarsRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
    }

    void KenwoodBarsRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        if (HasPeakTracker()) {
            GetPeakTracker().Resize(spectrum.size());
            GetPeakTracker().Update(spectrum, deltaTime);
        }
    }

    void KenwoodBarsRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        if (spectrum.empty()) return;

        const auto layout = CalculateBarLayout(
            spectrum.size(),
            m_settings.barSpacing
        );

        if (layout.barWidth <= 0.0f) return;

        canvas.DrawSpectrumBars(
            spectrum,
            GetViewportBounds(),
            CreateBarStyle(),
            GetPrimaryColor()
        );

        if (HasPeakTracker()) {
            const auto& tracker = GetPeakTracker();
            const float peakHeight = IsOverlay()
                ? kPeakHeightOverlay
                : kPeakHeight;
            const float cornerRadius = m_settings.cornerRadius * 0.5f;

            for (size_t i = 0; i < spectrum.size(); ++i) {
                if (tracker.IsPeakVisible(i)) {
                    const float peakY = MapToRange(
                        tracker.GetPeak(i),
                        0.0f,
                        1.0f,
                        static_cast<float>(GetHeight()),
                        0.0f
                    );

                    const Rect rect{
                        i * layout.totalBarWidth,
                        std::clamp(
                            peakY - peakHeight,
                            0.0f,
                            static_cast<float>(GetHeight())
                        ),
                        layout.barWidth,
                        peakHeight
                    };

                    if (cornerRadius > 0.0f) {
                        canvas.DrawRoundedRectangle(
                            rect,
                            cornerRadius,
                            Paint::Fill(Color::White())
                        );
                    }
                    else {
                        canvas.DrawRectangle(rect, Paint::Fill(Color::White()));
                    }
                }
            }
        }
    }

    BarStyle KenwoodBarsRenderer::CreateBarStyle() const {
        BarStyle style;
        style.spacing = m_settings.barSpacing;
        style.cornerRadius = m_settings.cornerRadius;
        style.useGradient = m_settings.useGradient;

        if (style.useGradient) {
            const auto gradient = CreateGradient(
                AdjustBrightness(
                    AdjustSaturation(GetPrimaryColor(), 0.8f),
                    0.5f
                ),
                AdjustBrightness(
                    AdjustSaturation(GetPrimaryColor(), 1.0f),
                    1.2f
                ),
                kGradientSteps
            );

            style.gradientStops.reserve(gradient.size());
            for (size_t i = 0; i < gradient.size(); ++i) {
                const float t = static_cast<float>(i) / (gradient.size() - 1);
                const auto& c = gradient[i];
                style.gradientStops.push_back({
                    t,
                    D2D1::ColorF(c.r, c.g, c.b, c.a)
                    });
            }
        }

        return style;
    }

} // namespace Spectrum