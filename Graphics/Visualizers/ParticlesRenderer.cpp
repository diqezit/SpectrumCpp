// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ParticlesRenderer for particle-based visualization
//
// Optimized implementation with zero per-frame allocations:
// - Reusable batch buffers cleared and repopulated each frame
// - Pre-calculated lookup tables for smooth animations
// - Direct array indexing instead of map allocations
// - Particle reserve strategy to minimize vector growth
// - Uses GeometryHelpers for all geometric operations
//
// Memory safety:
// - All batches reused (no new/delete per frame)
// - Lookup tables created once at initialization
// - Particle vector grows to max then stabilizes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/ParticlesRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Geometry;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kParticleVelocityMin = 8.0f;
        constexpr float kParticleVelocityMax = 35.0f;
        constexpr float kParticleLife = 2.0f;
        constexpr float kParticleLifeDecay = 1.2f;

        constexpr float kUpperBoundFactor = 0.5f;

        constexpr float kSpawnThresholdOverlay = 0.02f;
        constexpr float kSpawnThresholdNormal = 0.01f;
        constexpr float kSpawnProbability = 0.95f;
        constexpr float kMaxDensityFactor = 2.5f;

        constexpr float kParticleSizeOverlay = 2.5f;
        constexpr float kParticleSizeNormal = 3.0f;
        constexpr float kSizeDecayFactor = 0.992f;
        constexpr float kMinParticleSize = 0.3f;

        constexpr float kAlphaDecayExponent = 2.0f;

        constexpr int kVelocityLookupSize = 1024;
        constexpr int kAlphaCurveSize = 101;

        constexpr int kSizeBuckets = 12;
        constexpr int kAlphaBuckets = 12;
        constexpr int kTotalBatches = kSizeBuckets * kAlphaBuckets;

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

    void ParticlesRenderer::OnActivate(int width, int height)
    {
        BaseRenderer::OnActivate(width, height);
        m_particles.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<ParticlesRenderer>(m_quality);

        m_particles.clear();
        m_particles.reserve(m_settings.maxParticles);
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
    // Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::EnsureInitialized()
    {
        if (!IsInitialized()) {
            InitializeLookupTables();
            InitializeBatchBuffers();
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

        for (int i = 0; i < kAlphaCurveSize; ++i) {
            const float t = Helpers::Math::Normalize(
                static_cast<float>(i),
                0.0f,
                static_cast<float>(kAlphaCurveSize - 1)
            );
            m_alphaCurve[i] = std::pow(t, kAlphaDecayExponent);
        }
    }

    void ParticlesRenderer::InitializeVelocityLookup()
    {
        m_velocityLookup.resize(kVelocityLookupSize);

        for (int i = 0; i < kVelocityLookupSize; ++i) {
            const float t = Helpers::Math::Normalize(
                static_cast<float>(i),
                0.0f,
                static_cast<float>(kVelocityLookupSize)
            );
            m_velocityLookup[i] = Helpers::Math::Lerp(kParticleVelocityMin, kParticleVelocityMax, t);
        }
    }

    void ParticlesRenderer::InitializeBatchBuffers()
    {
        m_batchBuffer.resize(kTotalBatches);

        for (auto& batch : m_batchBuffer) {
            batch.positions.reserve(128);
        }
    }

    bool ParticlesRenderer::IsInitialized() const
    {
        return !m_alphaCurve.empty() &&
            !m_velocityLookup.empty() &&
            !m_batchBuffer.empty();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Logic
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

            const float spawnChance = Helpers::Math::Clamp(intensity, 0.0f, 1.0f) *
                kSpawnProbability *
                m_settings.particleSize;

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
        const Point spawnPos = CalculateSpawnPosition(spectrumIndex, barWidth);
        const float intensity = GetIntensityMultiplier(magnitude);

        m_particles.push_back(CreateParticle(spawnPos, intensity));
    }

    ParticlesRenderer::Particle ParticlesRenderer::CreateParticle(
        const Point& spawnPos,
        float intensity
    ) const
    {
        Particle particle;
        particle.position = spawnPos;
        particle.velocity = CalculateParticleVelocity(intensity);
        particle.size = CalculateParticleSize(intensity);
        particle.life = kParticleLife;
        particle.alpha = 1.0f;
        return particle;
    }

    bool ParticlesRenderer::IsParticleAlive(const Particle& particle) const
    {
        return particle.life > 0.0f &&
            IsParticleInBounds(particle) &&
            particle.size >= kMinParticleSize;
    }

    bool ParticlesRenderer::ShouldSpawnParticle(float magnitude) const
    {
        return magnitude > GetSpawnThreshold();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Physics
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::UpdateParticlePosition(
        Particle& particle,
        float deltaTime
    ) const
    {
        const Point velocity = { 0.0f, -particle.velocity * deltaTime };
        particle.position = Add(particle.position, velocity);
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
        const float lifeRatio = Helpers::Math::Clamp(
            particle.life / kParticleLife,
            0.0f,
            1.0f
        );

        particle.alpha = GetAlphaFromCurve(lifeRatio);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::RenderAllParticles(Canvas& canvas) const
    {
        if (m_particles.empty()) return;

        if (m_settings.useTrails) {
            RenderParticlesBatched(canvas);
        }
        else {
            RenderParticlesIndividual(canvas);
        }
    }

    void ParticlesRenderer::RenderParticlesBatched(Canvas& canvas) const
    {
        PrepareParticleBatches();

        for (const auto& batch : m_batchBuffer) {
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
        const float radius = particle.size * 0.5f;

        canvas.DrawCircle(particle.position, radius, Paint::Fill(color));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Batch Optimization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ParticlesRenderer::PrepareParticleBatches() const
    {
        ClearAllBatches();

        for (const auto& particle : m_particles) {
            if (!IsParticleVisible(particle)) continue;

            const int batchIndex = CalculateBatchIndex(particle);
            auto& batch = m_batchBuffer[batchIndex];

            if (batch.positions.empty()) {
                batch.size = particle.size;
                batch.color = CalculateParticleColor(particle);
            }

            batch.positions.push_back(particle.position);
        }
    }

    void ParticlesRenderer::ClearAllBatches() const
    {
        for (auto& batch : m_batchBuffer) {
            batch.Clear();
        }
    }

    int ParticlesRenderer::CalculateBatchIndex(const Particle& particle) const
    {
        const int sizeBucket = CalculateSizeBucket(particle.size);
        const int alphaBucket = CalculateAlphaBucket(particle.alpha);
        return sizeBucket * kAlphaBuckets + alphaBucket;
    }

    int ParticlesRenderer::CalculateSizeBucket(float size) const
    {
        const float normalizedSize = size / (GetBaseParticleSize() * kMaxDensityFactor);

        return Helpers::Math::Clamp(
            static_cast<int>(normalizedSize * kSizeBuckets),
            0,
            kSizeBuckets - 1
        );
    }

    int ParticlesRenderer::CalculateAlphaBucket(float alpha) const
    {
        return Helpers::Math::Clamp(
            static_cast<int>(alpha * kAlphaBuckets),
            0,
            kAlphaBuckets - 1
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Point ParticlesRenderer::CalculateSpawnPosition(
        size_t spectrumIndex,
        float barWidth
    ) const
    {
        const float baseX = spectrumIndex * barWidth;
        const float randomOffset = GetRandomNormalized() * barWidth;

        const Point spawnBase = GetSpawnPosition();
        const Point offset = { baseX + randomOffset, 0.0f };

        return Add(spawnBase, offset);
    }

    float ParticlesRenderer::CalculateParticleVelocity(float intensity) const
    {
        const float baseVelocity = GetRandomVelocity();

        const float clampedIntensity = Helpers::Math::Clamp(
            intensity,
            1.0f,
            kMaxDensityFactor
        );

        return baseVelocity * clampedIntensity;
    }

    float ParticlesRenderer::CalculateParticleSize(float intensity) const
    {
        const float baseSize = GetBaseParticleSize();

        const float clampedIntensity = Helpers::Math::Clamp(
            intensity,
            1.0f,
            kMaxDensityFactor
        );

        return baseSize * clampedIntensity * m_settings.trailLength;
    }

    Color ParticlesRenderer::CalculateParticleColor(const Particle& particle) const
    {
        return m_primaryColor.WithAlpha(particle.alpha);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Point ParticlesRenderer::GetSpawnPosition() const
    {
        const Rect bounds = CreateViewportBounds(m_width, m_height);
        return GetBottomLeft(bounds);
    }

    Point ParticlesRenderer::GetUpperBoundPosition() const
    {
        const float upperY = static_cast<float>(m_height) * kUpperBoundFactor;
        return { 0.0f, upperY };
    }

    float ParticlesRenderer::GetBarWidth(const SpectrumData& spectrum) const
    {
        if (spectrum.empty()) return 0.0f;
        return static_cast<float>(m_width) / spectrum.size();
    }

    Rect ParticlesRenderer::GetParticleBounds() const
    {
        const Point upperBound = GetUpperBoundPosition();
        const Point spawnPos = GetSpawnPosition();

        return CreateFromPoints(
            { 0.0f, upperBound.y },
            { static_cast<float>(m_width), spawnPos.y }
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lookup Tables
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::GetAlphaFromCurve(float lifeRatio) const
    {
        if (lifeRatio <= 0.0f) return 0.0f;
        if (lifeRatio >= 1.0f) return 1.0f;

        const float indexFloat = Helpers::Math::Map(
            lifeRatio,
            0.0f,
            1.0f,
            0.0f,
            static_cast<float>(kAlphaCurveSize - 1)
        );

        const int clampedIndex = Helpers::Math::Clamp(
            static_cast<int>(indexFloat),
            0,
            kAlphaCurveSize - 1
        );

        return m_alphaCurve[clampedIndex];
    }

    float ParticlesRenderer::GetRandomVelocity() const
    {
        const int index = static_cast<int>(
            GetRandomNormalized() * (kVelocityLookupSize - 1)
            );
        return m_velocityLookup[index];
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration
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
        return magnitude / GetSpawnThreshold();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
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
        const Rect bounds = GetParticleBounds();
        return Contains(bounds, particle.position);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Random Generation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ParticlesRenderer::GetRandomNormalized() const
    {
        return m_distribution(m_randomEngine);
    }

} // namespace Spectrum