#ifndef SPECTRUM_CPP_PARTICLES_RENDERER_H
#define SPECTRUM_CPP_PARTICLES_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the ParticlesRenderer for particle-based spectrum visualization.
//
// This renderer creates a physics-based particle system where particles
// spawn from the bottom based on spectrum intensity and rise upward with
// realistic motion simulation, creating a fountain-like effect.
//
// Key features:
// - Physics-based particle movement with velocity and decay
// - Lifetime management with smooth alpha curves
// - Quality-dependent particle count and batch rendering
// - Pre-calculated lookup tables for performance
// - Particle grouping by visual properties for batch drawing
//
// Design notes:
// - All rendering methods are const (state in m_particles)
// - High particle density with low rise height for dense effect
// - Batch rendering groups particles to minimize draw calls
// - Pre-calculated curves cached for smooth animation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include <vector>
#include <random>
#include <map>

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
        // Data Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings
        {
            int maxParticles;
            float spawnRate;
            float particleDetail;
            bool useBatchRendering;
        };

        struct Particle
        {
            float x = 0.0f;
            float y = 0.0f;
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
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void EnsureInitialized();
        void InitializeLookupTables();
        void InitializeAlphaCurve();
        void InitializeVelocityLookup();

        [[nodiscard]] bool IsInitialized() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Update Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateParticles(float deltaTime);
        void SpawnParticles(const SpectrumData& spectrum);
        void RemoveDeadParticles();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Particle Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSingleParticle(
            Particle& particle,
            float deltaTime
        );

        void SpawnParticleAt(
            size_t spectrumIndex,
            float magnitude,
            float barWidth
        );

        [[nodiscard]] Particle CreateParticle(
            float x,
            float intensity
        ) const;

        [[nodiscard]] bool IsParticleAlive(const Particle& particle) const;
        [[nodiscard]] bool ShouldSpawnParticle(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Particle Physics
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateParticlePosition(
            Particle& particle,
            float deltaTime
        ) const;

        void UpdateParticleLife(
            Particle& particle,
            float deltaTime
        ) const;

        void UpdateParticleSize(Particle& particle) const;
        void UpdateParticleAlpha(Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Rendering Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderAllParticles(Canvas& canvas) const;
        void RenderParticlesBatched(Canvas& canvas) const;
        void RenderParticlesIndividual(Canvas& canvas) const;

        void RenderSingleParticle(
            Canvas& canvas,
            const Particle& particle
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Batch Optimization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<ParticleBatch> GroupParticlesIntoBatches() const;

        [[nodiscard]] int CalculateSizeBucket(float size) const;
        [[nodiscard]] int CalculateAlphaBucket(float alpha) const;
        [[nodiscard]] int CalculateBatchKey(
            int sizeBucket,
            int alphaBucket
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float CalculateParticleX(
            size_t spectrumIndex,
            float barWidth
        ) const;

        [[nodiscard]] float CalculateParticleVelocity(float intensity) const;
        [[nodiscard]] float CalculateParticleSize(float intensity) const;

        [[nodiscard]] Color CalculateParticleColor(const Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetSpawnY() const;
        [[nodiscard]] float GetUpperBound() const;
        [[nodiscard]] float GetBarWidth(const SpectrumData& spectrum) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Alpha & Velocity Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetAlphaFromCurve(float lifeRatio) const;
        [[nodiscard]] float CalculateAlphaFallback(float lifeRatio) const;
        [[nodiscard]] float GetRandomVelocity() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetSpawnThreshold() const;
        [[nodiscard]] float GetBaseParticleSize() const;
        [[nodiscard]] float GetIntensityMultiplier(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CanSpawnParticles() const;
        [[nodiscard]] bool IsParticleVisible(const Particle& particle) const;
        [[nodiscard]] bool IsParticleInBounds(const Particle& particle) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Random Number Generation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetRandomNormalized() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        std::vector<Particle> m_particles;
        std::vector<float> m_alphaCurve;
        std::vector<float> m_velocityLookup;
        mutable std::mt19937 m_randomEngine;
        mutable std::uniform_real_distribution<float> m_distribution;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PARTICLES_RENDERER_H