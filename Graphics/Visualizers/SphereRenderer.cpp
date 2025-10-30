#include "Graphics/Visualizers/SphereRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Math;
    using namespace Helpers::Geometry;

    SphereRenderer::SphereRenderer()
        : m_sphereCount(0)
        , m_sphereRadius(0.0f)
    {
        UpdateSettings();
    }

    void SphereRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
        m_sphereCount = 0;
    }

    void SphereRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        const size_t requiredCount = CalculateSphereCount(spectrum);
        UpdateConfiguration(requiredCount);

        if (m_sphereCount == 0) return;

        const size_t count = std::min(m_sphereCount, spectrum.size());
        for (size_t i = 0; i < count; ++i) {
            const float targetAlpha = std::max(
                kMinAlpha,
                spectrum[i] * kAlphaMultiplier
            );

            m_currentAlphas[i] = SmoothValue(
                m_currentAlphas[i],
                targetAlpha,
                m_settings.rotationSpeed,
                0.95f
            );
        }
    }

    void SphereRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        if (m_sphereCount == 0) return;

        const auto spheres = CollectVisibleSpheres(spectrum);
        if (spheres.empty()) return;

        PointBatch sphereBatches;
        for (const auto& sphere : spheres) {
            sphereBatches[sphere.color].push_back(sphere.position);
        }

        for (const auto& [color, positions] : sphereBatches) {
            if (!positions.empty()) {
                const float radius = CalculateSphereSize(color.a);
                canvas.DrawCircleBatch(
                    positions,
                    radius,
                    Paint::Fill(color)
                );
            }
        }
    }

    void SphereRenderer::UpdateConfiguration(
        size_t requiredCount
    ) {
        if (requiredCount == m_sphereCount) return;

        m_sphereCount = requiredCount;
        m_sphereRadius = IsOverlay()
            ? kBaseRadiusOverlay
            : kBaseRadius;

        if (m_currentAlphas.size() < m_sphereCount) {
            m_currentAlphas.resize(m_sphereCount, kMinAlpha);
        }

        const float orbitRadius = GetMaxRadius() - m_sphereRadius;
        const Point center = GetViewportCenter();

        m_orbitPositions = GetCircularPoints(
            center,
            orbitRadius,
            m_sphereCount
        );
    }

    std::vector<SphereRenderer::SphereData>
        SphereRenderer::CollectVisibleSpheres(
            const SpectrumData& spectrum
        ) const {
        std::vector<SphereData> spheres;
        spheres.reserve(m_sphereCount);

        const size_t count = std::min(m_sphereCount, spectrum.size());

        for (size_t i = 0; i < count; ++i) {
            const float alpha = m_currentAlphas[i];
            const float size = CalculateSphereSize(alpha);

            if (size < kMinCircleSize) continue;

            SphereData sphere;
            sphere.position = m_orbitPositions[i];
            sphere.radius = size * 0.5f;
            sphere.color = CalculateSphereColor(alpha);

            spheres.push_back(sphere);
        }

        return spheres;
    }

    size_t SphereRenderer::CalculateSphereCount(
        const SpectrumData& spectrum
    ) const {
        const size_t qualityBars = static_cast<size_t>(
            RenderUtils::GetMaxBarsForQuality(GetQuality())
            );

        size_t count = Clamp(
            qualityBars,
            static_cast<size_t>(kMinSphereCount),
            static_cast<size_t>(kMaxSphereCount)
        );

        return std::min(count, spectrum.size());
    }

    Point SphereRenderer::CalculateSpherePosition(
        size_t index,
        float orbitRadius
    ) const {
        if (index >= m_orbitPositions.size()) {
            return GetViewportCenter();
        }
        return m_orbitPositions[index];
    }

    float SphereRenderer::CalculateSphereSize(
        float alpha
    ) const {
        return std::max(
            alpha * m_sphereRadius,
            kMinCircleSize
        );
    }

    Color SphereRenderer::CalculateSphereColor(
        float alpha
    ) const {
        return AdjustAlpha(GetPrimaryColor(), alpha);
    }

} // namespace Spectrum