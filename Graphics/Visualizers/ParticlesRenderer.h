#ifndef SPECTRUM_CPP_PARTICLES_RENDERER_H
#define SPECTRUM_CPP_PARTICLES_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ParticlesRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

#include <algorithm>

namespace Spectrum {

    class ParticlesRenderer final : public BaseRenderer<ParticlesRenderer> {
    public:
        ParticlesRenderer() { UpdateSettings(); }

        [[nodiscard]] std::string_view GetName() const override { return "Particles"; }

        void OnActivate(int width, int height) override {
            BaseRenderer::OnActivate(width, height);
            m_particles.clear();
        }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::ParticlesSettings>();
            m_particles.clear();
            m_particles.reserve(static_cast<size_t>(m_settings.maxParticles));
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            for (auto& p : m_particles) {
                p.pos.y -= p.vel * dt;
                p.life -= 1.2f * dt;
                p.size *= 0.992f;
                p.alpha = std::pow(Clamp(p.life / 2.0f, 0.0f, 1.0f), 2.0f);
            }

            m_particles.erase(
                std::remove_if(m_particles.begin(), m_particles.end(),
                    [](const Particle& p) {
                        return p.life <= 0.0f || p.pos.y < 0.0f || p.size < 0.3f;
                    }),
                m_particles.end());

            Spawn(spectrum);
        }

        void DoRender(BLContext& ctx, const SpectrumData&) override {
            for (const auto& p : m_particles) {
                if (p.alpha <= 0.0f || p.size <= 0.0f) continue;
                Draw::FillCircle(ctx, p.pos, p.size * 0.5f,
                    AdjustAlpha(GetPrimaryColor(), p.alpha));
            }
        }

    private:
        struct Particle {
            Point pos{};
            float vel = 0.0f;
            float size = 0.0f;
            float life = 0.0f;
            float alpha = 0.0f;
        };

        void Spawn(const SpectrumData& spectrum) {
            const size_t cap = static_cast<size_t>(m_settings.maxParticles);
            if (m_particles.size() >= cap || spectrum.empty()) return;

            const float threshold = IsOverlay() ? 0.02f : 0.01f;
            const float baseSize = IsOverlay() ? 2.5f : 3.0f;
            const float barW = static_cast<float>(GetWidth()) / static_cast<float>(spectrum.size());
            auto& rng = Helpers::Utils::Random::Instance();

            for (size_t i = 0; i < spectrum.size() && m_particles.size() < cap; ++i) {
                if (spectrum[i] <= threshold) continue;

                const float intensity = spectrum[i] / threshold;
                if (rng.Float() >= Clamp(intensity, 0.0f, 1.0f) * 0.95f * m_settings.particleSize)
                    continue;

                Particle p;
                p.pos = { static_cast<float>(i) * barW + rng.Float() * barW, static_cast<float>(GetHeight()) };
                p.vel = Lerp(8.0f, 35.0f, rng.Float()) * Clamp(intensity, 1.0f, 2.5f);
                p.size = baseSize * Clamp(intensity, 1.0f, 2.5f) * m_settings.trailLength;
                p.life = 2.0f;
                p.alpha = 1.0f;
                m_particles.push_back(p);
            }
        }

        Settings::ParticlesSettings m_settings{};
        std::vector<Particle> m_particles;
    };

} // namespace Spectrum

#endif