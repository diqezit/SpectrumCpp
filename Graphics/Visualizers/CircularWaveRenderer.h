#ifndef SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H
#define SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CircularWaveRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class CircularWaveRenderer final : public BaseRenderer<CircularWaveRenderer> {
    public:
        CircularWaveRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Circular Wave"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::CircularWaveSettings>();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            const float intensity = RenderUtils::GetAverageMagnitude(spectrum);
            m_angle += m_settings.rotationSpeed * (1.0f + intensity * 0.3f) * dt;
            if (m_angle > Helpers::Constants::kTwoPi)
                m_angle -= Helpers::Constants::kTwoPi;
            m_waveTime += m_settings.waveSpeed * dt;
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            const int rings = std::min(static_cast<int>(spectrum.size()), m_settings.maxRings);
            if (rings <= 0) return;

            const Point center = GetViewportCenter();
            const float maxRadius = GetMaxRadius();
            const float step = (maxRadius - kCenterRadius) / static_cast<float>(rings);

            for (int i = rings - 1; i >= 0; --i) {
                const float mag = RenderUtils::SegmentAverage(
                    spectrum, static_cast<size_t>(rings), static_cast<size_t>(i));
                if (mag < kMinMagnitude) continue;

                const float radius = kCenterRadius
                    + static_cast<float>(i) * step
                    + std::sin(m_waveTime + static_cast<float>(i) * 0.1f + m_angle) * mag * step;
                if (radius <= 0.0f || radius > maxRadius) continue;

                const float stroke = std::clamp(1.5f + mag * 6.0f, 1.5f, m_settings.maxStroke);
                const Color color = AdjustAlpha(
                    GetPrimaryColor(),
                    Saturate(mag * 1.5f * (1.0f - radius / maxRadius)));

                if (m_settings.useGlow && mag > kGlowThreshold) {
                    Draw::Glow(ctx, center, radius + stroke,
                        AdjustAlpha(color, color.a * 0.5f), mag * 0.8f);
                }
                Draw::StrokeCircle(ctx, center, radius, color, stroke);
            }
        }

    private:
        static constexpr float kCenterRadius = 30.0f;
        static constexpr float kMinMagnitude = 0.01f;
        static constexpr float kGlowThreshold = 0.5f;

        Settings::CircularWaveSettings m_settings{};
        float m_angle = 0.0f;
        float m_waveTime = 0.0f;
    };

} // namespace Spectrum

#endif