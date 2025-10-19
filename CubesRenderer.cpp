// CubesRenderer.cpp

#include "CubesRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "RenderUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr float MIN_MAGNITUDE = 0.01f;
        constexpr float SPACING = 2.0f;
        constexpr float HEIGHT_SCALE = 0.9f;
        constexpr float SHADOW_OFFSET = 3.0f;
        constexpr float SHADOW_ALPHA = 0.2f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor & Settings
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    CubesRenderer::CubesRenderer() {
        m_primaryColor = Color::FromRGB(200, 100, 255);
        UpdateSettings();
    }

    void CubesRenderer::UpdateSettings() {
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Core Render Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CubesRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        if (spectrum.empty()) {
            return;
        }

        auto layout = RenderUtils::ComputeBarLayout(
            spectrum.size(),
            SPACING,
            m_width
        );

        if (layout.barWidth <= 0.0f) {
            return;
        }

        // Pre-calculate cube geometries
        std::vector<CubeGeometry> cubes;
        cubes.reserve(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            float magnitude = spectrum[i];

            if (magnitude < MIN_MAGNITUDE) {
                continue;
            }

            CubeGeometry cube = CalculateCubeGeometry(
                i,
                magnitude,
                layout
            );
            cubes.push_back(cube);
        }

        if (cubes.empty()) {
            return;
        }

        // Render with or without shadows
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
    ) const {
        CubeGeometry cube;

        float height = RenderUtils::MagnitudeToHeight(
            magnitude,
            m_height,
            HEIGHT_SCALE
        );

        cube.frontFace = Rect(
            index * layout.totalBarWidth,
            m_height - height,
            layout.barWidth,
            height
        );

        cube.topHeight = layout.barWidth * m_settings.topHeightRatio;
        cube.sideWidth = layout.barWidth * m_settings.perspective;
        cube.magnitude = magnitude;

        return cube;
    }

    std::vector<Point> CubesRenderer::GetSideFacePoints(
        const CubeGeometry& cube
    ) const {
        Point p1 = { cube.frontFace.GetRight(), cube.frontFace.y };
        Point p2 = { p1.x + cube.sideWidth, p1.y - cube.topHeight };
        Point p3 = { p2.x, p2.y + cube.frontFace.height };
        Point p4 = { p1.x, cube.frontFace.GetBottom() };

        return { p1, p2, p3, p4 };
    }

    std::vector<Point> CubesRenderer::GetTopFacePoints(
        const CubeGeometry& cube
    ) const {
        Point p1 = { cube.frontFace.x, cube.frontFace.y };
        Point p2 = { cube.frontFace.GetRight(), cube.frontFace.y };
        Point p3 = { p2.x + cube.sideWidth, p2.y - cube.topHeight };
        Point p4 = { p1.x + cube.sideWidth, p1.y - cube.topHeight };

        return { p1, p2, p3, p4 };
    }

    Color CubesRenderer::GetCubeColor(float magnitude) const {
        Color color = m_primaryColor;
        color.a = 0.6f + 0.4f * magnitude;
        return color;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CubesRenderer::RenderCubes(
        GraphicsContext& context,
        const std::vector<CubeGeometry>& cubes
    ) const {
        // Render in correct order for 3D effect
        // 1. Side faces (back to front)
        if (m_settings.useSideFace) {
            for (const auto& cube : cubes) {
                RenderSideFace(context, cube);
            }
        }

        // 2. Top faces
        if (m_settings.useTopFace) {
            for (const auto& cube : cubes) {
                RenderTopFace(context, cube);
            }
        }

        // 3. Front faces (can use batch rendering)
        RenderFrontFaces(context, cubes);
    }

    void CubesRenderer::RenderCubesWithShadows(
        GraphicsContext& context,
        const std::vector<CubeGeometry>& cubes
    ) const {
        // Draw shadows first
        auto drawShadows = [&]() {
            RenderCubes(context, cubes);
            };

        context.DrawWithShadow(
            drawShadows,
            { SHADOW_OFFSET, SHADOW_OFFSET },
            0.0f,
            Color(0.0f, 0.0f, 0.0f, SHADOW_ALPHA)
        );
    }

    void CubesRenderer::RenderSideFace(
        GraphicsContext& context,
        const CubeGeometry& cube
    ) const {
        Color baseColor = GetCubeColor(cube.magnitude);
        Color sideColor = Utils::AdjustBrightness(
            baseColor,
            m_settings.sideFaceBrightness
        );

        auto points = GetSideFacePoints(cube);
        context.DrawPolygon(points, sideColor, true);
    }

    void CubesRenderer::RenderTopFace(
        GraphicsContext& context,
        const CubeGeometry& cube
    ) const {
        Color baseColor = GetCubeColor(cube.magnitude);
        Color topColor = Utils::AdjustBrightness(baseColor, 1.2f);

        auto points = GetTopFacePoints(cube);
        context.DrawPolygon(points, topColor, true);
    }

    void CubesRenderer::RenderFrontFaces(
        GraphicsContext& context,
        const std::vector<CubeGeometry>& cubes
    ) const {
        // Group cubes by similar color for batch rendering
        std::vector<Rect> frontFaces;
        frontFaces.reserve(cubes.size());

        for (const auto& cube : cubes) {
            frontFaces.push_back(cube.frontFace);
        }

        // For simple case: if all have same base color, use batch
        if (!frontFaces.empty()) {
            // Draw individually with per-cube alpha
            for (const auto& cube : cubes) {
                Color color = GetCubeColor(cube.magnitude);
                context.DrawRectangle(cube.frontFace, color, true);
            }
        }
    }

} // namespace Spectrum