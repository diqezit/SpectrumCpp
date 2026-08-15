#ifndef SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H
#define SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PolylineWaveRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class PolylineWaveRenderer final : public BaseRenderer<PolylineWaveRenderer> {
    public:
        PolylineWaveRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Sunburst"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::PolylineWaveSettings>();
            m_core = m_target = 0.08f;
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            if (!m_settings.useFill || spectrum.empty()) return;

            float sum = 0.0f;
            for (float v : spectrum) sum += v;

            m_target = Lerp(0.08f, 0.15f, Saturate(sum / static_cast<float>(spectrum.size())));
            m_core = Lerp(m_core, m_target, Clamp(0.1f * dt * 60.0f, 0.0f, 1.0f));
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (spectrum.empty()) return;

            if (m_dirs.size() != spectrum.size())
                m_dirs = GetCircularPoints({ 0.0f, 0.0f }, 1.0f, spectrum.size());

            const Point center = GetViewportCenter();
            const float radius = GetMinDimension() * 0.35f;
            const float width = Clamp(
                Helpers::Constants::kTwoPi * radius / static_cast<float>(spectrum.size()) * 0.75f,
                2.0f, 15.0f);

            if (m_settings.useFill) {
                Draw::FillCircle(ctx, center, radius * m_core,
                    AdjustAlpha(GetPrimaryColor(), 0.3f));
            }

            const Color color = GetPrimaryColor();
            for (size_t i = 0; i < spectrum.size(); ++i) {
                if (spectrum[i] < 0.02f) continue;
                const float len = std::max(spectrum[i] * radius * 0.6f, radius * 0.05f);
                Draw::StrokeLine(ctx,
                    Helpers::Geometry::Add(center, Helpers::Geometry::Multiply(m_dirs[i], radius)),
                    Helpers::Geometry::Add(center, Helpers::Geometry::Multiply(m_dirs[i], radius + len)),
                    color, width);
            }
        }

    private:
        Settings::PolylineWaveSettings m_settings{};
        std::vector<Point> m_dirs;
        float m_core = 0.08f;
        float m_target = 0.08f;
    };

} // namespace Spectrum

#endif