#include "Graphics/Visualizers/ParticlesRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum {

    using namespace Helpers::Math;

    namespace {
        constexpr float kParticleVelocityMin = 8.0f;
        constexpr float kParticleVelocityMax = 35.0f;
        constexpr float kParticleLife = 2.0f;
        constexpr float kParticleLifeDecay = 1.2f;
        constexpr float kSpawnThresholdOverlay = 0.02f;
        constexpr float kSpawnThresholdNormal = 0.01f;
        constexpr float kParticleSizeOverlay = 2.5f;
        constexpr float kParticleSizeNormal = 3.0f;
        constexpr float kSizeDecayFactor = 0.992f;
        constexpr float kMinParticleSize = 0.3f;
    }

    ParticlesRenderer::ParticlesRenderer()
        : m_randomEngine(std::random_device{}())
        , m_distribution(0.0f, 1.0f)
    {
        UpdateSettings();
    }

    void ParticlesRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        m_particles.clear();
    }

    void ParticlesRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
        m_particles.clear();
        m_particles.reserve(m_settings.maxParticles);
    }

    void ParticlesRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        UpdateParticles(deltaTime);
        SpawnParticles(spectrum);
    }

    void ParticlesRenderer::DoRender(Canvas& canvas, const SpectrumData&) {
        for (const auto& particle : m_particles) {
            if (particle.alpha > 0.0f && particle.size > 0.0f) {
                const Color color = AdjustAlpha(
                    GetPrimaryColor(),
                    particle.alpha
                );
                canvas.DrawCircle(
                    particle.position,
                    particle.size * 0.5f,
                    Paint::Fill(color)
                );
            }
        }
    }

    void ParticlesRenderer::UpdateParticles(float deltaTime) {
        for (auto& particle : m_particles) {
            particle.position.y -= particle.velocity * deltaTime;
            particle.life -= kParticleLifeDecay * deltaTime;
            particle.size *= kSizeDecayFactor;
            particle.alpha = std::pow(
                Clamp(particle.life / kParticleLife, 0.0f, 1.0f),
                2.0f
            );
        }

        m_particles.erase(
            std::remove_if(
                m_particles.begin(),
                m_particles.end(),
                [](const Particle& p) {
                    return p.life <= 0.0f ||
                        p.position.y < 0.0f ||
                        p.size < kMinParticleSize;
                }
            ),
            m_particles.end()
        );
    }

    void ParticlesRenderer::SpawnParticles(const SpectrumData& spectrum) {
        if (m_particles.size() >= static_cast<size_t>(m_settings.maxParticles)) {
            return;
        }

        const float threshold = IsOverlay()
            ? kSpawnThresholdOverlay
            : kSpawnThresholdNormal;

        const float baseSize = IsOverlay()
            ? kParticleSizeOverlay
            : kParticleSizeNormal;

        const float barWidth = spectrum.empty()
            ? 0.0f
            : static_cast<float>(GetWidth()) / spectrum.size();

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (m_particles.size() >= static_cast<size_t>(m_settings.maxParticles)) {
                break;
            }

            if (spectrum[i] <= threshold) continue;

            const float intensity = spectrum[i] / threshold;
            const float spawnChance = Clamp(intensity, 0.0f, 1.0f) *
                0.95f * m_settings.particleSize;

            if (GetRandomNormalized() < spawnChance) {
                Particle p;
                p.position = {
                    i * barWidth + GetRandomNormalized() * barWidth,
                    static_cast<float>(GetHeight())
                };
                p.velocity = Lerp(
                    kParticleVelocityMin,
                    kParticleVelocityMax,
                    GetRandomNormalized()
                ) * Clamp(intensity, 1.0f, 2.5f);
                p.size = baseSize * Clamp(intensity, 1.0f, 2.5f) *
                    m_settings.trailLength;
                p.life = kParticleLife;
                p.alpha = 1.0f;
                m_particles.push_back(p);
            }
        }
    }

    float ParticlesRenderer::GetRandomNormalized() const {
        return m_distribution(m_randomEngine);
    }

} // namespace Spectrum