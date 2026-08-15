#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// WaveRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class WaveRenderer final : public BaseRenderer<WaveRenderer> {
    public:
        WaveRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Wave"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::WaveSettings>();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            if (spectrum.empty()) return;
            m_intensity = Lerp(
                m_intensity,
                RenderUtils::GetAverageMagnitude(spectrum),
                Clamp(0.15f * dt * 60.0f, 0.0f, 1.0f));
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (spectrum.empty()) return;

            const Rect bounds = GetViewportBounds();
            const float width = 2.0f * m_settings.waveHeight;
            Color color = GetPrimaryColor();

            if (m_intensity > 0.7f) {
                color = AdjustBrightness(color,
                    Lerp(1.0f, 1.3f, Map(m_intensity, 0.7f, 1.0f, 0.0f, 1.0f)));
            }

            if (m_settings.useFill && m_settings.useMirror) {
                RenderWithShadow(ctx,
                    [&]() { Stroke(ctx, spectrum, bounds, color, width, false); },
                    { 0.0f, 3.0f }, 0.5f);
            }

            if (m_settings.useFill) {
                const int layers = std::max(1, m_settings.points / 64);
                for (int i = layers; i >= 1; --i) {
                    Color glow = GetPrimaryColor();
                    const float t = Normalize(static_cast<float>(i), 0.0f, static_cast<float>(layers));
                    glow.a *= (0.4f / static_cast<float>(i))
                        * (1.0f + t * 0.5f)
                        * m_settings.smoothness
                        * Lerp(1.0f, 1.2f, m_intensity);

                    const float glowW = width + static_cast<float>(i) * 2.5f;
                    Stroke(ctx, spectrum, bounds, glow, glowW, false);
                    if (m_settings.useMirror) {
                        glow.a *= 0.55f;
                        Stroke(ctx, spectrum, bounds, glow, glowW, true);
                    }
                }
            }

            Stroke(ctx, spectrum, bounds, color, width, false);
            if (m_settings.useMirror) {
                color.a *= 0.45f;
                Stroke(ctx, spectrum, bounds, color, width, true);
            }
        }

    private:
        void Stroke(
            BLContext& ctx,
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float width,
            bool reflected) const
        {
            std::vector<Point> points;
            RenderUtils::BuildPolylineFromSpectrum(
                spectrum,
                bounds.y + bounds.height * 0.5f,
                bounds.height * 0.5f * (reflected ? -1.0f : 1.0f),
                static_cast<int>(bounds.width),
                points);
            if (bounds.x != 0.0f) {
                for (auto& p : points) p.x += bounds.x;
            }
            Draw::StrokePolyline(ctx, points, color, width);
        }

        Settings::WaveSettings m_settings{};
        float m_intensity = 0.0f;
    };

} // namespace Spectrum

#endif