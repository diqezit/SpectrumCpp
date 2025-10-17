// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ParticlesRenderer for particle-based visualization.
//
// Implementation details:
// - High-density particle spawning creates fountain-like effect
// - Physics simulation with slow upward movement and gradual decay
// - Low rise height keeps particles concentrated near bottom
// - Batch rendering groups particles by size/alpha for performance
// - Pre-calculated curves for smooth alpha transitions
// - Velocity lookup table for consistent randomization
//
// Rendering pipeline:
// 1. Update physics: position, life, size, alpha for all particles
// 2. Cleanup: remove dead particles (life <= 0, out of bounds)
// 3. Spawning: create new particles based on spectrum intensity
// 4. Batching: group particles by visual properties
// 5. Drawing: render batches with single draw call per group
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/ParticlesRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        // Physics parameters (low values for minimal rise)
        constexpr float kParticleVelocityMin = 8.0f;
        constexpr float kParticleVelocityMax = 35.0f;
        constexpr float kParticleLife = 2.0f;
        constexpr float kParticleLifeDecay = 1.2f;

        // Boundary limits
        constexpr float kUpperBoundFactor = 0.5f;  // Half-screen height

        // Spawn thresholds (very low for maximum density)
        constexpr float kSpawnThresholdOverlay = 0.02f;
        constexpr float kSpawnThresholdNormal = 0.01f;
        constexpr float kSpawnProbability = 0.95f;
        constexpr float kMaxDensityFactor = 2.5f;

        // Particle appearance
        constexpr float kParticleSizeOverlay = 2.5f;
        constexpr float kParticleSizeNormal = 3.0f;
        constexpr float kSizeDecayFactor = 0.992f;
        constexpr float kMinParticleSize = 0.3f;

        // Alpha decay
        constexpr float kAlphaDecayExponent = 2.0f;

        // Lookup table dimensions
        constexpr int kVelocityLookupSize = 1024;
        constexpr int kAlphaCurveSize = 101;

        // Quality-based particle limits (ultra high density)
        constexpr int kMaxParticlesLow = 5000;
        constexpr int kMaxParticlesMedium = 10000;
        constexpr int kMaxParticlesHigh = 15000;

        // Batch grouping granularity
        constexpr int kSizeBuckets = 12;
        constexpr int kAlphaBuckets = 12;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ParticlesRenderer::ParticlesRenderer()
        : m_randomEngine(std::random_device{}())
        , m_distribution(0.0f, 1.0f)
    {
        m_primaryColor = Color::FromRGB(100, 200, 255);
        UpdateSettings();
    }

    void ParticlesRenderer::OnActivate(
        int width,
        int height
    )
    {
        BaseRenderer::OnActivate(width, height);
        m_particles.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = {
                kMaxParticlesLow,     // maxParticles
                0.7f,                 // spawnRate
                0.6f,                 // particleDetail
                true                  // useBatchRendering
            };
            break;
        case RenderQuality::High:
            m_settings = {
                kMaxParticlesHigh,    // maxParticles
                1.0f,                 // spawnRate
                1.0f,                 // particleDetail
                true                  // useBatchRendering
            };
            break;
        case RenderQuality::Medium:
        default:
            m_settings = {
                kMaxParticlesMedium,  // maxParticles
                0.85f,                // spawnRate
                0.8f,                 // particleDetail
                true                  // useBatchRendering
            };
            break;
        }

        m_particles.clear();
    }

    void ParticlesRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        EnsureInitialized();
        UpdateParticles(deltaTime);
        SpawnParticles(spectrum);
    }

    void ParticlesRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        RenderAllParticles(canvas);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialization Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::EnsureInitialized()
    {
        if (!IsInitialized()) {
            InitializeLookupTables();
        }
    }

    void ParticlesRenderer::InitializeLookupTables()
    {
        InitializeAlphaCurve();
        InitializeVelocityLookup();
    }

    void ParticlesRenderer::InitializeAlphaCurve()
    {
        m_alphaCurve.resize(kAlphaCurveSize);

        const float step = 1.0f / (kAlphaCurveSize - 1);

        for (int i = 0; i < kAlphaCurveSize; ++i) {
            const float t = i * step;
            m_alphaCurve[i] = std::pow(t, kAlphaDecayExponent);
        }
    }

    void ParticlesRenderer::InitializeVelocityLookup()
    {
        m_velocityLookup.resize(kVelocityLookupSize);

        const float velocityRange = kParticleVelocityMax - kParticleVelocityMin;

        for (int i = 0; i < kVelocityLookupSize; ++i) {
            const float t = static_cast<float>(i) / kVelocityLookupSize;
            m_velocityLookup[i] = kParticleVelocityMin + velocityRange * t;
        }
    }

    bool ParticlesRenderer::IsInitialized() const
    {
        return !m_alphaCurve.empty() && !m_velocityLookup.empty();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Update Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::UpdateParticles(float deltaTime)
    {
        for (auto& particle : m_particles) {
            UpdateSingleParticle(particle, deltaTime);
        }

        RemoveDeadParticles();
    }

    void ParticlesRenderer::SpawnParticles(const SpectrumData& spectrum)
    {
        if (!CanSpawnParticles()) return;

        const float barWidth = GetBarWidth(spectrum);
        const float threshold = GetSpawnThreshold();

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (m_particles.size() >= static_cast<size_t>(m_settings.maxParticles)) {
                break;
            }

            if (!ShouldSpawnParticle(spectrum[i])) continue;

            const float intensity = spectrum[i] / threshold;
            const float spawnChance = Utils::Clamp(intensity, 0.0f, 1.0f)
                * kSpawnProbability
                * m_settings.spawnRate;

            if (GetRandomNormalized() < spawnChance) {
                SpawnParticleAt(i, spectrum[i], barWidth);
            }
        }
    }

    void ParticlesRenderer::RemoveDeadParticles()
    {
        m_particles.erase(
            std::remove_if(
                m_particles.begin(),
                m_particles.end(),
                [this](const Particle& p) { return !IsParticleAlive(p); }
            ),
            m_particles.end()
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Particle Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::UpdateSingleParticle(
        Particle& particle,
        float deltaTime
    )
    {
        UpdateParticlePosition(particle, deltaTime);
        UpdateParticleLife(particle, deltaTime);
        UpdateParticleSize(particle);
        UpdateParticleAlpha(particle);
    }

    void ParticlesRenderer::SpawnParticleAt(
        size_t spectrumIndex,
        float magnitude,
        float barWidth
    )
    {
        const float x = CalculateParticleX(spectrumIndex, barWidth);
        const float intensity = GetIntensityMultiplier(magnitude);

        m_particles.push_back(CreateParticle(x, intensity));
    }

    ParticlesRenderer::Particle ParticlesRenderer::CreateParticle(
        float x,
        float intensity
    ) const
    {
        Particle particle;
        particle.x = x;
        particle.y = GetSpawnY();
        particle.velocity = CalculateParticleVelocity(intensity);
        particle.size = CalculateParticleSize(intensity);
        particle.life = kParticleLife;
        particle.alpha = 1.0f;

        return particle;
    }

    bool ParticlesRenderer::IsParticleAlive(const Particle& particle) const
    {
        if (particle.life <= 0.0f) return false;
        if (!IsParticleInBounds(particle)) return false;
        if (particle.size < kMinParticleSize) return false;

        return true;
    }

    bool ParticlesRenderer::ShouldSpawnParticle(float magnitude) const
    {
        return magnitude > GetSpawnThreshold();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Particle Physics
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::UpdateParticlePosition(
        Particle& particle,
        float deltaTime
    ) const
    {
        particle.y -= particle.velocity * deltaTime;
    }

    void ParticlesRenderer::UpdateParticleLife(
        Particle& particle,
        float deltaTime
    ) const
    {
        particle.life -= kParticleLifeDecay * deltaTime;
    }

    void ParticlesRenderer::UpdateParticleSize(Particle& particle) const
    {
        particle.size *= kSizeDecayFactor;
    }

    void ParticlesRenderer::UpdateParticleAlpha(Particle& particle) const
    {
        const float lifeRatio = Utils::Clamp(
            particle.life / kParticleLife,
            0.0f,
            1.0f
        );

        particle.alpha = GetAlphaFromCurve(lifeRatio);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Rendering Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::RenderAllParticles(Canvas& canvas) const
    {
        if (m_particles.empty()) return;

        if (m_settings.useBatchRendering) {
            RenderParticlesBatched(canvas);
        }
        else {
            RenderParticlesIndividual(canvas);
        }
    }

    void ParticlesRenderer::RenderParticlesBatched(Canvas& canvas) const
    {
        const auto batches = GroupParticlesIntoBatches();

        for (const auto& batch : batches) {
            if (!batch.positions.empty()) {
                canvas.DrawCircleBatch(
                    batch.positions,
                    batch.size * 0.5f,
                    Paint::Fill(batch.color)
                );
            }
        }
    }

    void ParticlesRenderer::RenderParticlesIndividual(Canvas& canvas) const
    {
        for (const auto& particle : m_particles) {
            if (IsParticleVisible(particle)) {
                RenderSingleParticle(canvas, particle);
            }
        }
    }

    void ParticlesRenderer::RenderSingleParticle(
        Canvas& canvas,
        const Particle& particle
    ) const
    {
        const Color color = CalculateParticleColor(particle);
        const Point center = { particle.x, particle.y };
        const float radius = particle.size * 0.5f;

        canvas.DrawCircle(center, radius, Paint::Fill(color));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Batch Optimization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    std::vector<ParticlesRenderer::ParticleBatch>
        ParticlesRenderer::GroupParticlesIntoBatches() const
    {
        std::map<int, ParticleBatch> batchMap;

        for (const auto& particle : m_particles) {
            if (!IsParticleVisible(particle)) continue;

            const int sizeBucket = CalculateSizeBucket(particle.size);
            const int alphaBucket = CalculateAlphaBucket(particle.alpha);
            const int key = CalculateBatchKey(sizeBucket, alphaBucket);

            auto& batch = batchMap[key];

            if (batch.positions.empty()) {
                batch.size = particle.size;
                batch.color = CalculateParticleColor(particle);
            }

            batch.positions.push_back({ particle.x, particle.y });
        }

        std::vector<ParticleBatch> batches;
        batches.reserve(batchMap.size());

        for (auto& [key, batch] : batchMap) {
            batches.push_back(std::move(batch));
        }

        return batches;
    }

    int ParticlesRenderer::CalculateSizeBucket(float size) const
    {
        const float normalizedSize = size / (GetBaseParticleSize() * kMaxDensityFactor);

        return Utils::Clamp(
            static_cast<int>(normalizedSize * kSizeBuckets),
            0,
            kSizeBuckets - 1
        );
    }

    int ParticlesRenderer::CalculateAlphaBucket(float alpha) const
    {
        return Utils::Clamp(
            static_cast<int>(alpha * kAlphaBuckets),
            0,
            kAlphaBuckets - 1
        );
    }

    int ParticlesRenderer::CalculateBatchKey(
        int sizeBucket,
        int alphaBucket
    ) const
    {
        return sizeBucket * kAlphaBuckets + alphaBucket;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::CalculateParticleX(
        size_t spectrumIndex,
        float barWidth
    ) const
    {
        const float baseX = spectrumIndex * barWidth;
        const float randomOffset = GetRandomNormalized() * barWidth;

        return baseX + randomOffset;
    }

    float ParticlesRenderer::CalculateParticleVelocity(float intensity) const
    {
        const float baseVelocity = GetRandomVelocity();
        const float clampedIntensity = Utils::Clamp(
            intensity,
            1.0f,
            kMaxDensityFactor
        );

        return baseVelocity * clampedIntensity;
    }

    float ParticlesRenderer::CalculateParticleSize(float intensity) const
    {
        const float baseSize = GetBaseParticleSize();
        const float clampedIntensity = Utils::Clamp(
            intensity,
            1.0f,
            kMaxDensityFactor
        );

        return baseSize * clampedIntensity * m_settings.particleDetail;
    }

    Color ParticlesRenderer::CalculateParticleColor(const Particle& particle) const
    {
        return m_primaryColor.WithAlpha(particle.alpha);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::GetSpawnY() const
    {
        return static_cast<float>(m_height);
    }

    float ParticlesRenderer::GetUpperBound() const
    {
        return static_cast<float>(m_height) * kUpperBoundFactor;
    }

    float ParticlesRenderer::GetBarWidth(const SpectrumData& spectrum) const
    {
        if (spectrum.empty()) return 0.0f;

        return static_cast<float>(m_width) / spectrum.size();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Alpha & Velocity Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::GetAlphaFromCurve(float lifeRatio) const
    {
        if (lifeRatio <= 0.0f) return 0.0f;
        if (lifeRatio >= 1.0f) return 1.0f;

        if (m_alphaCurve.empty()) {
            return CalculateAlphaFallback(lifeRatio);
        }

        const int index = static_cast<int>(lifeRatio * (kAlphaCurveSize - 1));
        const int clampedIndex = Utils::Clamp(index, 0, kAlphaCurveSize - 1);

        return m_alphaCurve[clampedIndex];
    }

    float ParticlesRenderer::CalculateAlphaFallback(float lifeRatio) const
    {
        return std::pow(lifeRatio, kAlphaDecayExponent);
    }

    float ParticlesRenderer::GetRandomVelocity() const
    {
        if (m_velocityLookup.empty()) {
            return kParticleVelocityMin;
        }

        const int index = static_cast<int>(
            GetRandomNormalized() * (kVelocityLookupSize - 1)
            );

        return m_velocityLookup[index];
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::GetSpawnThreshold() const
    {
        return m_isOverlay ? kSpawnThresholdOverlay : kSpawnThresholdNormal;
    }

    float ParticlesRenderer::GetBaseParticleSize() const
    {
        return m_isOverlay ? kParticleSizeOverlay : kParticleSizeNormal;
    }

    float ParticlesRenderer::GetIntensityMultiplier(float magnitude) const
    {
        const float threshold = GetSpawnThreshold();
        return magnitude / threshold;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool ParticlesRenderer::CanSpawnParticles() const
    {
        return m_particles.size() < static_cast<size_t>(m_settings.maxParticles);
    }

    bool ParticlesRenderer::IsParticleVisible(const Particle& particle) const
    {
        return particle.alpha > 0.0f && particle.size > 0.0f;
    }

    bool ParticlesRenderer::IsParticleInBounds(const Particle& particle) const
    {
        return particle.y >= GetUpperBound() && particle.y <= GetSpawnY();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Random Number Generation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::GetRandomNormalized() const
    {
        return m_distribution(m_randomEngine);
    }

} // namespace Spectrum