#ifndef SPECTRUM_CPP_SPHERE_RENDERER_H
#define SPECTRUM_CPP_SPHERE_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class SphereRenderer final : public BaseRenderer<SphereRenderer>
    {
    public:
        SphereRenderer();
        ~SphereRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Sphere;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Sphere";
        }

    protected:
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
        using Settings = Settings::SphereSettings;

        static constexpr float kMinAlpha = 0.1f;
        static constexpr float kMinCircleSize = 2.0f;
        static constexpr float kBaseRadius = 40.0f;
        static constexpr float kBaseRadiusOverlay = 20.0f;
        static constexpr float kAlphaMultiplier = 3.0f;
        static constexpr int kMinSphereCount = 8;
        static constexpr int kMaxSphereCount = 64;

        struct SphereData {
            Point position;
            float radius;
            Color color;
        };

        void UpdateConfiguration(size_t requiredCount);

        [[nodiscard]] std::vector<SphereData> CollectVisibleSpheres(
            const SpectrumData& spectrum
        ) const;

        [[nodiscard]] size_t CalculateSphereCount(
            const SpectrumData& spectrum
        ) const;

        [[nodiscard]] Point CalculateSpherePosition(
            size_t index,
            float orbitRadius
        ) const;

        [[nodiscard]] float CalculateSphereSize(
            float alpha
        ) const;

        [[nodiscard]] Color CalculateSphereColor(
            float alpha
        ) const;

        Settings m_settings;
        size_t m_sphereCount;
        float m_sphereRadius;
        std::vector<Point> m_orbitPositions;
        std::vector<float> m_currentAlphas;
    };

} // namespace Spectrum

#endif