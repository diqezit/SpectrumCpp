#ifndef SPECTRUM_CPP_PARTICLES_RENDERER_H
#define SPECTRUM_CPP_PARTICLES_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>
#include <random>

namespace Spectrum {

    class Canvas;

    class ParticlesRenderer final : public BaseRenderer<ParticlesRenderer>
    {
    public:
        ParticlesRenderer();
        ~ParticlesRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Particles;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Particles";
        }

        void OnActivate(int width, int height) override;

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(Canvas& canvas, const SpectrumData& spectrum) override;

    private:
        using Settings = Settings::ParticlesSettings;

        struct Particle {
            Point position = { 0.0f, 0.0f };
            float velocity = 0.0f;
            float size = 0.0f;
            float life = 0.0f;
            float alpha = 0.0f;
        };

        void UpdateParticles(float deltaTime);
        void SpawnParticles(const SpectrumData& spectrum);

        [[nodiscard]] float GetRandomNormalized() const;

        Settings m_settings;
        std::vector<Particle> m_particles;
        mutable std::mt19937 m_randomEngine;
        mutable std::uniform_real_distribution<float> m_distribution;
    };

} // namespace Spectrum

#endif