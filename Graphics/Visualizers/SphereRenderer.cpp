// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the SphereRenderer for orbital sphere visualization.
//
// Implementation details:
// - Spheres orbit in a circle, each representing a frequency band
// - Size and opacity smoothly animated based on audio magnitude
// - Batch rendering groups spheres with similar alpha values
// - Adaptive sphere count scales with quality and spectrum size
// - Trigonometry pre-calculated once per configuration change
//
// Rendering pipeline:
// 1. Configuration: update sphere count and radius if needed
// 2. Animation: smooth alpha transitions with asymmetric response
// 3. Grouping: batch spheres by alpha similarity
// 4. Drawing: render batches with solid or gradient fills
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/SphereRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Brushes/GradientStop.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include "Common/Types.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        // Visual parameters
        constexpr float kMinMagnitude = 0.01f;
        constexpr float kMaxIntensityMultiplier = 3.0f;
        constexpr float kMinAlpha = 0.1f;
        constexpr float kMinCircleSize = 2.0f;
        constexpr float kBaseRadius = 40.0f;
        constexpr float kBaseRadiusOverlay = 20.0f;

        // Alpha grouping thresholds
        constexpr float kAlphaThreshold = 0.1f;
        constexpr int kMaxAlphaGroups = 5;

        // Sphere count limits
        constexpr int kMinSphereCount = 8;
        constexpr int kMaxSphereCount = 64;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    SphereRenderer::SphereRenderer()
        : m_sphereCount(0)
        , m_sphereRadius(0.0f)
    {
        m_primaryColor = Color::FromRGB(0, 150, 255);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SphereRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { false, 0.15f };
            break;
        case RenderQuality::High:
            m_settings = { true, 0.25f };
            break;
        case RenderQuality::Medium:
        default:
            m_settings = { true, 0.2f };
            break;
        }

        // Force reconfiguration on next update
        m_sphereCount = 0;
    }

    void SphereRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float /*deltaTime*/
    )
    {
        UpdateConfiguration(spectrum);

        if (m_sphereCount == 0) return;

        UpdateAlphas(spectrum);
    }

    void SphereRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        if (m_sphereCount == 0) return;

        const auto groups = GroupAlphas();

        if (m_settings.useGradient) {
            RenderGradientSpheres(canvas, groups);
        }
        else {
            RenderSolidSpheres(canvas, groups);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Rendering Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SphereRenderer::RenderSolidSpheres(
        Canvas& canvas,
        const std::vector<AlphaGroup>& groups
    ) const
    {
        for (const auto& group : groups) {
            if (group.alpha < kMinAlpha) continue;

            const Color color = m_primaryColor.WithAlpha(group.alpha);
            RenderGroup(canvas, group, Paint::Fill(color));
        }
    }

    void SphereRenderer::RenderGradientSpheres(
        Canvas& canvas,
        const std::vector<AlphaGroup>& groups
    ) const
    {
        for (const auto& group : groups) {
            if (group.alpha < kMinAlpha) continue;

            const Color centerColor = m_primaryColor.WithAlpha(group.alpha);
            const Color edgeColor = m_primaryColor.WithAlpha(0.0f);

            const std::vector<GradientStop> stops = {
                {0.0f, centerColor},
                {1.0f, edgeColor}
            };

            const Paint paint = Paint::RadialGradient(
                { 0.0f, 0.0f },
                1.0f,
                stops
            );

            RenderGroup(canvas, group, paint);
        }
    }

    void SphereRenderer::RenderGroup(
        Canvas& canvas,
        const AlphaGroup& group,
        const Paint& paint
    ) const
    {
        for (size_t i = group.start; i < group.end; ++i) {
            RenderSingleSphere(canvas, i, paint);
        }
    }

    void SphereRenderer::RenderSingleSphere(
        Canvas& canvas,
        size_t index,
        const Paint& paint
    ) const
    {
        const float size = GetSphereSize(index);
        if (size < kMinCircleSize) return;

        const Point pos = GetSpherePosition(index);
        const float radius = size * 0.5f;

        if (m_settings.useGradient) {
            // Scale gradient to sphere size
            canvas.PushTransform();
            canvas.TranslateBy(pos.x, pos.y);
            canvas.ScaleAt({ 0.0f, 0.0f }, radius, radius);
            canvas.DrawCircle({ 0.0f, 0.0f }, 1.0f, paint);
            canvas.PopTransform();
        }
        else {
            canvas.DrawCircle(pos, radius, paint);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SphereRenderer::UpdateConfiguration(const SpectrumData& spectrum)
    {
        const size_t qualityBars = static_cast<size_t>(
            RenderUtils::GetMaxBarsForQuality(m_quality)
            );

        size_t count = Utils::Clamp(
            qualityBars,
            static_cast<size_t>(kMinSphereCount),
            static_cast<size_t>(kMaxSphereCount)
        );

        count = std::min(count, spectrum.size());

        if (count != m_sphereCount) {
            m_sphereCount = count;
            m_sphereRadius = m_isOverlay ? kBaseRadiusOverlay : kBaseRadius;

            EnsureArraysInitialized();
            PrecomputeTrigValues();
        }
    }

    void SphereRenderer::UpdateAlphas(const SpectrumData& spectrum)
    {
        const size_t count = std::min(m_sphereCount, spectrum.size());

        for (size_t i = 0; i < count; ++i) {
            const float target = std::max(
                kMinAlpha,
                spectrum[i] * kMaxIntensityMultiplier
            );

            m_currentAlphas[i] = Utils::Lerp(
                m_currentAlphas[i],
                target,
                m_settings.responseSpeed
            );
        }
    }

    void SphereRenderer::EnsureArraysInitialized()
    {
        if (m_cosValues.size() < m_sphereCount) {
            m_cosValues.resize(m_sphereCount);
            m_sinValues.resize(m_sphereCount);
            m_currentAlphas.resize(m_sphereCount, kMinAlpha);
        }
    }

    void SphereRenderer::PrecomputeTrigValues()
    {
        if (m_sphereCount == 0) return;

        const float angleStep = TWO_PI / m_sphereCount;

        for (size_t i = 0; i < m_sphereCount; ++i) {
            const float angle = i * angleStep;
            m_cosValues[i] = std::cos(angle);
            m_sinValues[i] = std::sin(angle);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    std::vector<SphereRenderer::AlphaGroup> SphereRenderer::GroupAlphas() const
    {
        if (m_currentAlphas.empty()) return {};

        std::vector<AlphaGroup> groups;
        groups.reserve(kMaxAlphaGroups);

        size_t currentStart = 0;
        float currentAlpha = m_currentAlphas[0];

        for (size_t i = 1; i < m_sphereCount; ++i) {
            const bool shouldSplit =
                std::abs(m_currentAlphas[i] - currentAlpha) > kAlphaThreshold ||
                groups.size() >= kMaxAlphaGroups - 1;

            if (shouldSplit) {
                groups.push_back({ currentStart, i, currentAlpha });
                currentStart = i;
                currentAlpha = m_currentAlphas[i];
            }
        }

        groups.push_back({ currentStart, m_sphereCount, currentAlpha });
        return groups;
    }

    Point SphereRenderer::GetSpherePosition(size_t index) const
    {
        const float radius = GetMaxRadius();

        return {
            m_width * 0.5f + m_cosValues[index] * radius,
            m_height * 0.5f + m_sinValues[index] * radius
        };
    }

    float SphereRenderer::GetSphereSize(size_t index) const
    {
        return std::max(
            m_currentAlphas[index] * m_sphereRadius,
            kMinCircleSize
        );
    }

    float SphereRenderer::GetMaxRadius() const
    {
        return std::min(m_width, m_height) * 0.5f - m_sphereRadius;
    }

} // namespace Spectrum