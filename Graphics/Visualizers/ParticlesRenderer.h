#ifndef SPECTRUM_CPP_PARTICLES_RENDERER_H
#define SPECTRUM_CPP_PARTICLES_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the ParticlesRenderer for particle-based spectrum visualization
//
// Physics-based particle system with fountain-like effect from bottom
// Optimized for zero memory leaks with efficient batch rendering
// Uses GeometryHelpers for all geometric calculations
//
// Performance optimizations:
// - Reusable batch buffers (no per-frame allocations)
// - Pre-calculated lookup tables for alpha/velocity
// - Direct-indexed batching (no map allocations)
// - Particle pooling with reserve strategy
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>
#include <random>

namespace Spectrum {

    class Canvas;

    class ParticlesRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ParticlesRenderer();
        ~ParticlesRenderer() override = default;

        ParticlesRenderer(const ParticlesRenderer&) = delete;
        ParticlesRenderer& operator=(const ParticlesRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::Particles; }
        [[nodiscard]] std::string_view GetName() const override { return "Particles"; }

        void OnActivate(int width, int height) override;

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BaseRenderer Overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        using Settings = Settings::ParticlesSettings;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Data Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Particle
        {
            Point position = { 0.0f, 0.0f };
            float velocity = 0.0f;
            float size = 0.0f;
            float life = 0.0f;
            float alpha = 0.0f;
        };

        struct ParticleBatch
        {
            std::vector<Point> positions;
            float size = 0.0f;
            Color color;

            void Clear()
            {
                positions.clear();
            }
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void EnsureInitialized();
        void InitializeLookupTables();
        void InitializeAlphaCurve();
        void InitializeVelocityLookup();
        void InitializeBatchBuffers();

        [[nodiscard]] bool IsInitialized() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Update Logic
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateParticles(float deltaTime);
        void SpawnParticles(const SpectrumData& spectrum);
        void RemoveDeadParticles();

        void UpdateSingleParticle(Particle& particle, float deltaTime);
        void SpawnParticleAt(size_t spectrumIndex, float magnitude, float barWidth);

        [[nodiscard]] Particle CreateParticle(const Point& spawnPos, float intensity) const;
        [[nodiscard]] bool IsParticleAlive(const Particle& particle) const;
        [[nodiscard]] bool ShouldSpawnParticle(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Physics
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateParticlePosition(Particle& particle, float deltaTime) const;
        void UpdateParticleLife(Particle& particle, float deltaTime) const;
        void UpdateParticleSize(Particle& particle) const;
        void UpdateParticleAlpha(Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderAllParticles(Canvas& canvas) const;
        void RenderParticlesBatched(Canvas& canvas) const;
        void RenderParticlesIndividual(Canvas& canvas) const;
        void RenderSingleParticle(Canvas& canvas, const Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Batch Optimization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void PrepareParticleBatches() const;
        void ClearAllBatches() const;

        [[nodiscard]] int CalculateBatchIndex(const Particle& particle) const;
        [[nodiscard]] int CalculateSizeBucket(float size) const;
        [[nodiscard]] int CalculateAlphaBucket(float alpha) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Point CalculateSpawnPosition(
            size_t spectrumIndex,
            float barWidth
        ) const;

        [[nodiscard]] float CalculateParticleVelocity(float intensity) const;
        [[nodiscard]] float CalculateParticleSize(float intensity) const;
        [[nodiscard]] Color CalculateParticleColor(const Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Point GetSpawnPosition() const;
        [[nodiscard]] Point GetUpperBoundPosition() const;
        [[nodiscard]] float GetBarWidth(const SpectrumData& spectrum) const;
        [[nodiscard]] Rect GetParticleBounds() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lookup Tables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetAlphaFromCurve(float lifeRatio) const;
        [[nodiscard]] float GetRandomVelocity() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetSpawnThreshold() const;
        [[nodiscard]] float GetBaseParticleSize() const;
        [[nodiscard]] float GetIntensityMultiplier(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CanSpawnParticles() const;
        [[nodiscard]] bool IsParticleVisible(const Particle& particle) const;
        [[nodiscard]] bool IsParticleInBounds(const Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Random Generation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetRandomNormalized() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;

        std::vector<Particle> m_particles;
        std::vector<float> m_alphaCurve;
        std::vector<float> m_velocityLookup;

        mutable std::vector<ParticleBatch> m_batchBuffer;
        mutable std::mt19937 m_randomEngine;
        mutable std::uniform_real_distribution<float> m_distribution;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PARTICLES_RENDERER_H