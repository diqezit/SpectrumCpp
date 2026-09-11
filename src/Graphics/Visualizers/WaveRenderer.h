#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// WaveRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class WaveRenderer final : public BaseRenderer<WaveRenderer> {
    public:
        WaveRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Wave"; }

    protected:
        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            if (spectrum.empty()) return;
            m_intensity = Lerp(
                m_intensity,
                RenderUtils::GetAverageMagnitude(spectrum),
                SmoothDt(0.15f, dt));
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (spectrum.empty()) return;

            const Rect bounds = GetViewportBounds();
            const float width = 2.0f * m_settings.waveHeight;
            Color color = RenderUtils::IntensityColor(GetPrimaryColor(), m_intensity);

            if (m_settings.useFill && m_settings.useMirror) {
                RenderWithShadow(ctx,
                    [&]() { Stroke(ctx, spectrum, bounds, color, width, false); },
                    { 0.0f, 3.0f }, 0.5f);
            }

            if (m_settings.useFill)
                StrokeGlow(ctx, spectrum, bounds, width);

            Stroke(ctx, spectrum, bounds, color, width, false);
            if (m_settings.useMirror) {
                color.a *= 0.45f;
                Stroke(ctx, spectrum, bounds, color, width, true);
            }
        }

    private:
        void StrokeGlow(
            BLContext& ctx,
            const SpectrumData& spectrum,
            const Rect& bounds,
            float width) const
        {
            const int layers = std::max(1, m_settings.points / 64);
            for (int i = layers; i >= 1; --i) {
                Color glow = GetPrimaryColor();
                const float t = Normalize(float(i), 0.0f, float(layers));
                glow.a *= (0.4f / float(i))
                    * (1.0f + t * 0.5f)
                    * m_settings.smoothness
                    * Lerp(1.0f, 1.2f, m_intensity);

                const float glowW = width + float(i) * 2.5f;
                Stroke(ctx, spectrum, bounds, glow, glowW, false);
                if (m_settings.useMirror) {
                    glow.a *= 0.55f;
                    Stroke(ctx, spectrum, bounds, glow, glowW, true);
                }
            }
        }

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
                int(bounds.width),
                points);
            if (bounds.x != 0.0f)
                OffsetPoints(points, bounds.x, 0.0f);
            Draw::StrokePolyline(ctx, points, color, width);
        }

        float m_intensity = 0.0f;
    };

} // namespace Spectrum

#endif