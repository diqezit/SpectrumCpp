#ifndef SPECTRUM_CPP_SPHERE_RENDERER_H
#define SPECTRUM_CPP_SPHERE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the SphereRenderer for orbital sphere visualization.
//
// This renderer displays spectrum data as a circle of spheres, with each
// sphere's size and opacity driven by its corresponding frequency band.
// Creates a planetary system effect with spheres orbiting the center.
//
// Key features:
// - Spheres arranged in circular orbit pattern
// - Size and alpha modulation based on audio magnitude
// - Batch rendering by grouping similar alpha values
// - Adaptive sphere count based on spectrum size
// - Pre-calculated trigonometry for performance
//
// Design notes:
// - All rendering methods are const (state in m_currentAlphas)
// - Quality settings control gradient effects and response speed
// - Groups spheres by alpha to reduce draw calls
// - Supports both solid and radial gradient rendering
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/API/Structs/Paint.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class SphereRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        SphereRenderer();
        ~SphereRenderer() override = default;

        SphereRenderer(const SphereRenderer&) = delete;
        SphereRenderer& operator=(const SphereRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::Sphere; }
        [[nodiscard]] std::string_view GetName() const override { return "Sphere"; }

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
            bool useGradient;
            float responseSpeed;
        };

        struct AlphaGroup
        {
            size_t start;
            size_t end;
            float alpha;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Rendering Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderSolidSpheres(
            Canvas& canvas,
            const std::vector<AlphaGroup>& groups
        ) const;

        void RenderGradientSpheres(
            Canvas& canvas,
            const std::vector<AlphaGroup>& groups
        ) const;

        void RenderGroup(
            Canvas& canvas,
            const AlphaGroup& group,
            const Paint& paint
        ) const;

        void RenderSingleSphere(
            Canvas& canvas,
            size_t index,
            const Paint& paint
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateConfiguration(const SpectrumData& spectrum);
        void UpdateAlphas(const SpectrumData& spectrum);
        void EnsureArraysInitialized();
        void PrecomputeTrigValues();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<AlphaGroup> GroupAlphas() const;
        [[nodiscard]] Point GetSpherePosition(size_t index) const;
        [[nodiscard]] float GetSphereSize(size_t index) const;
        [[nodiscard]] float GetMaxRadius() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        size_t m_sphereCount;
        float m_sphereRadius;
        std::vector<float> m_cosValues;
        std::vector<float> m_sinValues;
        std::vector<float> m_currentAlphas;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SPHERE_RENDERER_H