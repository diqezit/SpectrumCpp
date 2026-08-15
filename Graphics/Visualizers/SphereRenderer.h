#ifndef SPECTRUM_CPP_SPHERE_RENDERER_H
#define SPECTRUM_CPP_SPHERE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SphereRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class SphereRenderer final : public BaseRenderer<SphereRenderer> {
    public:
        SphereRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Sphere"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::SphereSettings>();
            m_count = 0;
        }

        void UpdateAnimation(const SpectrumData& spectrum, float) override {
            const size_t want = std::min(
                Clamp(static_cast<size_t>(RenderUtils::GetMaxBarsForQuality(GetQuality())),
                    size_t{ 8 }, size_t{ 64 }),
                spectrum.size());

            if (want != m_count) {
                m_count = want;
                m_radius = IsOverlay() ? 20.0f : 40.0f;
                if (m_alpha.size() < m_count)
                    m_alpha.resize(m_count, 0.1f);
                m_orbit = GetCircularPoints(GetViewportCenter(), GetMaxRadius() - m_radius, m_count);
            }
            if (m_count == 0) return;

            const size_t n = std::min(m_count, spectrum.size());
            for (size_t i = 0; i < n; ++i) {
                m_alpha[i] = SmoothValue(
                    m_alpha[i], std::max(0.1f, spectrum[i] * 3.0f),
                    m_settings.rotationSpeed, 0.95f);
            }
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (m_count == 0) return;

            const size_t n = std::min(m_count, spectrum.size());
            for (size_t i = 0; i < n; ++i) {
                const float size = std::max(m_alpha[i] * m_radius, 2.0f);
                if (size < 2.0f) continue;
                Draw::FillCircle(ctx, m_orbit[i], size * 0.5f,
                    AdjustAlpha(GetPrimaryColor(), m_alpha[i]));
            }
        }

    private:
        Settings::SphereSettings m_settings{};
        size_t m_count = 0;
        float m_radius = 0.0f;
        std::vector<Point> m_orbit;
        std::vector<float> m_alpha;
    };

} // namespace Spectrum

#endif