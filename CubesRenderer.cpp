// CubesRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the CubesRenderer for 3D-style bar visualization.
//
// Implementation details:
// - Pre-calculates all cube geometries before rendering
// - Rendering order: shadows -> side faces -> top faces -> front faces
// - Side faces darker than front (simulated lighting)
// - Top faces lighter than front (simulated lighting)
// - Uses D2DHelpers for sanitization and validation
//
// Performance optimizations:
// - Pre-calculation pass filters out invisible cubes
// - Batch rendering for front faces (when possible)
// - Skip rendering faces below visibility threshold
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "CubesRenderer.h"
#include "D2DHelpers.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "RenderUtils.h"

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kMinMagnitude = 0.01f;
        constexpr float kSpacing = 2.0f;
        constexpr float kHeightScale = 0.9f;
        constexpr float kShadowOffset = 3.0f;
        constexpr float kShadowAlpha = 0.2f;
        constexpr float kTopBrightness = 1.2f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    CubesRenderer::CubesRenderer()
    {
        m_primaryColor = Color::FromRGB(200, 100, 255);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CubesRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { false, true, false, 0.2f, 0.7f, 0.15f };
            break;
        case RenderQuality::High:
            m_settings = { true, true, true, 0.3f, 0.5f, 0.35f };
            break;
        case RenderQuality::Medium:
        default:
            m_settings = { true, true, true, 0.25f, 0.6f, 0.25f };
            break;
        }
    }

    void CubesRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    )
    {
        if (spectrum.empty()) return;

        const auto layout = RenderUtils::ComputeBarLayout(
            spectrum.size(),
            kSpacing,
            m_width
        );

        if (layout.barWidth <= 0.0f) return;

        std::vector<CubeGeometry> cubes;
        cubes.reserve(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float magnitude = Sanitize::NormalizedFloat(spectrum[i]);

            if (magnitude < kMinMagnitude) continue;

            cubes.push_back(CalculateCubeGeometry(i, magnitude, layout));
        }

        if (cubes.empty()) return;

        if (m_settings.useShadow) {
            RenderCubesWithShadows(context, cubes);
        }
        else {
            RenderCubes(context, cubes);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    CubesRenderer::CubeGeometry CubesRenderer::CalculateCubeGeometry(
        size_t index,
        float magnitude,
        const RenderUtils::BarLayout& layout
    ) const
    {
        CubeGeometry cube;

        const float height = RenderUtils::MagnitudeToHeight(
            magnitude,
            m_height,
            kHeightScale
        );

        cube.frontFace = Rect{
            index * layout.totalBarWidth,
            m_height - height,
            layout.barWidth,
            height
        };

        cube.topHeight = layout.barWidth * m_settings.topHeightRatio;
        cube.sideWidth = layout.barWidth * m_settings.perspective;
        cube.magnitude = magnitude;

        return cube;
    }

    std::vector<Point> CubesRenderer::GetSideFacePoints(
        const CubeGeometry& cube
    ) const
    {
        const Point p1 = { cube.frontFace.GetRight(), cube.frontFace.y };
        const Point p2 = { p1.x + cube.sideWidth, p1.y - cube.topHeight };
        const Point p3 = { p2.x, p2.y + cube.frontFace.height };
        const Point p4 = { p1.x, cube.frontFace.GetBottom() };

        return { p1, p2, p3, p4 };
    }

    std::vector<Point> CubesRenderer::GetTopFacePoints(
        const CubeGeometry& cube
    ) const
    {
        const Point p1 = { cube.frontFace.x, cube.frontFace.y };
        const Point p2 = { cube.frontFace.GetRight(), cube.frontFace.y };
        const Point p3 = { p2.x + cube.sideWidth, p2.y - cube.topHeight };
        const Point p4 = { p1.x + cube.sideWidth, p1.y - cube.topHeight };

        return { p1, p2, p3, p4 };
    }

    Color CubesRenderer::GetCubeColor(float magnitude) const
    {
        Color color = m_primaryColor;
        color.a = 0.6f + 0.4f * magnitude;
        return color;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CubesRenderer::RenderCubes(
        GraphicsContext& context,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        if (m_settings.useSideFace) {
            for (const auto& cube : cubes) {
                RenderSideFace(context, cube);
            }
        }

        if (m_settings.useTopFace) {
            for (const auto& cube : cubes) {
                RenderTopFace(context, cube);
            }
        }

        RenderFrontFaces(context, cubes);
    }

    void CubesRenderer::RenderCubesWithShadows(
        GraphicsContext& context,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        auto drawShadows = [this, &context, &cubes]() {
            RenderCubes(context, cubes);
            };

        context.DrawWithShadow(
            drawShadows,
            { kShadowOffset, kShadowOffset },
            0.0f,
            Color(0.0f, 0.0f, 0.0f, kShadowAlpha)
        );
    }

    void CubesRenderer::RenderSideFace(
        GraphicsContext& context,
        const CubeGeometry& cube
    ) const
    {
        const Color baseColor = GetCubeColor(cube.magnitude);
        const Color sideColor = Utils::AdjustBrightness(
            baseColor,
            m_settings.sideFaceBrightness
        );

        const auto points = GetSideFacePoints(cube);
        context.DrawPolygon(points, sideColor, true);
    }

    void CubesRenderer::RenderTopFace(
        GraphicsContext& context,
        const CubeGeometry& cube
    ) const
    {
        const Color baseColor = GetCubeColor(cube.magnitude);
        const Color topColor = Utils::AdjustBrightness(baseColor, kTopBrightness);

        const auto points = GetTopFacePoints(cube);
        context.DrawPolygon(points, topColor, true);
    }

    void CubesRenderer::RenderFrontFaces(
        GraphicsContext& context,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        for (const auto& cube : cubes) {
            const Color color = GetCubeColor(cube.magnitude);
            context.DrawRectangle(cube.frontFace, color, true);
        }
    }

} // namespace Spectrum