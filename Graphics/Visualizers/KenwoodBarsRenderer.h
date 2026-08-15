#ifndef SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H
#define SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// KenwoodBarsRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class KenwoodBarsRenderer final : public BaseRenderer<KenwoodBarsRenderer> {
    public:
        KenwoodBarsRenderer() {
            InitializePeakTracker(0, 0.3f, 0.95f);
            UpdateSettings();
        }

        [[nodiscard]] std::string_view GetName() const override { return "Kenwood Bars"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::KenwoodBarsSettings>();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            if (!HasPeakTracker()) return;
            GetPeakTracker().Resize(spectrum.size());
            GetPeakTracker().Update(spectrum, dt);
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (spectrum.empty()) return;

            const auto layout = CalculateBarLayout(spectrum.size(), m_settings.barSpacing);
            if (layout.barWidth <= 0.0f) return;

            const float viewH = static_cast<float>(GetHeight());
            for (size_t i = 0; i < spectrum.size(); ++i) {
                const float mag = Helpers::Sanitize::Normalized(spectrum[i]);
                DrawRoundedRect(ctx, GetBarRect(layout, i, std::max(mag * viewH, 1.0f)),
                    m_settings.cornerRadius, BarColor(mag));
            }

            if (!HasPeakTracker()) return;

            const auto& peaks = GetPeakTracker();
            const float peakH = IsOverlay() ? 2.0f : 3.0f;
            for (size_t i = 0; i < spectrum.size(); ++i) {
                if (!peaks.IsPeakVisible(i)) continue;
                const float y = Map(peaks.GetPeak(i), 0.0f, 1.0f, viewH, 0.0f);
                DrawRoundedRect(ctx, {
                    static_cast<float>(i) * layout.totalBarWidth,
                    std::clamp(y - peakH, 0.0f, viewH),
                    layout.barWidth, peakH
                    }, m_settings.cornerRadius * 0.5f, Color::White());
            }
        }

    private:
        [[nodiscard]] Color BarColor(float mag) const {
            if (!m_settings.useGradient) return GetPrimaryColor();
            return InterpolateColor(
                AdjustBrightness(AdjustSaturation(GetPrimaryColor(), 0.8f), 0.5f),
                AdjustBrightness(AdjustSaturation(GetPrimaryColor(), 1.0f), 1.2f),
                mag);
        }

        Settings::KenwoodBarsSettings m_settings{};
    };

} // namespace Spectrum

#endif