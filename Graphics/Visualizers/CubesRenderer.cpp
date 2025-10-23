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

#include "Graphics/Visualizers/CubesRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kMinMagnitude = 0.01f;
        constexpr float kSpacing = 2.0f;
        constexpr float kHeightScale = 0.9f;

        constexpr float kShadowOffsetX = 3.0f;
        constexpr float kShadowOffsetY = 3.0f;
        constexpr float kShadowBlurRadius = 0.0f;
        constexpr float kShadowAlpha = 0.2f;

        constexpr float kTopBrightness = 1.2f;

        constexpr float kAlphaBase = 0.6f;
        constexpr float kAlphaRange = 0.4f;

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
        Canvas& canvas,
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

        CollectVisibleCubes(cubes, spectrum, layout);

        if (AreCubesEmpty(cubes)) return;

        if (m_settings.useShadow) {
            RenderCubesWithShadows(canvas, cubes);
        }
        else {
            RenderCubes(canvas, cubes);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Cube Collection
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CubesRenderer::CollectVisibleCubes(
        std::vector<CubeGeometry>& cubes,
        const SpectrumData& spectrum,
        const RenderUtils::BarLayout& layout
    ) const
    {
        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);

            if (!IsMagnitudeVisible(magnitude)) continue;

            cubes.push_back(CalculateCubeGeometry(i, magnitude, layout));
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

        const float height = CalculateBarHeight(magnitude);

        cube.frontFace = CalculateFrontFace(index, height, layout);
        cube.topHeight = CalculateTopHeight(layout.barWidth);
        cube.sideWidth = CalculateSideWidth(layout.barWidth);
        cube.magnitude = magnitude;

        return cube;
    }

    Rect CubesRenderer::CalculateFrontFace(
        size_t index,
        float height,
        const RenderUtils::BarLayout& layout
    ) const
    {
        return {
            index * layout.totalBarWidth,
            static_cast<float>(m_height) - height,
            layout.barWidth,
            height
        };
    }

    float CubesRenderer::CalculateBarHeight(float magnitude) const
    {
        return RenderUtils::MagnitudeToHeight(
            magnitude,
            m_height,
            kHeightScale
        );
    }

    float CubesRenderer::CalculateTopHeight(float barWidth) const
    {
        return barWidth * m_settings.topHeightRatio;
    }

    float CubesRenderer::CalculateSideWidth(float barWidth) const
    {
        return barWidth * m_settings.perspective;
    }

    std::vector<Point> CubesRenderer::GetSideFacePoints(
        const CubeGeometry& cube
    ) const
    {
        return {
            CalculateSidePoint1(cube),
            CalculateSidePoint2(cube),
            CalculateSidePoint3(cube),
            CalculateSidePoint4(cube)
        };
    }

    std::vector<Point> CubesRenderer::GetTopFacePoints(
        const CubeGeometry& cube
    ) const
    {
        return {
            CalculateTopPoint1(cube),
            CalculateTopPoint2(cube),
            CalculateTopPoint3(cube),
            CalculateTopPoint4(cube)
        };
    }

    Point CubesRenderer::CalculateSidePoint1(const CubeGeometry& cube) const
    {
        return { cube.frontFace.GetRight(), cube.frontFace.y };
    }

    Point CubesRenderer::CalculateSidePoint2(const CubeGeometry& cube) const
    {
        const Point p1 = CalculateSidePoint1(cube);
        return { p1.x + cube.sideWidth, p1.y - cube.topHeight };
    }

    Point CubesRenderer::CalculateSidePoint3(const CubeGeometry& cube) const
    {
        const Point p2 = CalculateSidePoint2(cube);
        return { p2.x, p2.y + cube.frontFace.height };
    }

    Point CubesRenderer::CalculateSidePoint4(const CubeGeometry& cube) const
    {
        return { cube.frontFace.GetRight(), cube.frontFace.GetBottom() };
    }

    Point CubesRenderer::CalculateTopPoint1(const CubeGeometry& cube) const
    {
        return { cube.frontFace.x, cube.frontFace.y };
    }

    Point CubesRenderer::CalculateTopPoint2(const CubeGeometry& cube) const
    {
        return { cube.frontFace.GetRight(), cube.frontFace.y };
    }

    Point CubesRenderer::CalculateTopPoint3(const CubeGeometry& cube) const
    {
        const Point p2 = CalculateTopPoint2(cube);
        return { p2.x + cube.sideWidth, p2.y - cube.topHeight };
    }

    Point CubesRenderer::CalculateTopPoint4(const CubeGeometry& cube) const
    {
        const Point p1 = CalculateTopPoint1(cube);
        return { p1.x + cube.sideWidth, p1.y - cube.topHeight };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Face Rendering (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CubesRenderer::RenderCubes(
        Canvas& canvas,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        RenderAllFaces(canvas, cubes);
    }

    void CubesRenderer::RenderCubesWithShadows(
        Canvas& canvas,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        auto drawCubeShapes = [this, &canvas, &cubes]() {
            RenderAllFaces(canvas, cubes);
            };

        canvas.DrawWithShadow(
            drawCubeShapes,
            { kShadowOffsetX, kShadowOffsetY },
            kShadowBlurRadius,
            Color(0.0f, 0.0f, 0.0f, kShadowAlpha)
        );

        RenderAllFaces(canvas, cubes);
    }

    void CubesRenderer::RenderAllFaces(
        Canvas& canvas,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        if (ShouldRenderSideFaces()) {
            RenderSideFaces(canvas, cubes);
        }

        if (ShouldRenderTopFaces()) {
            RenderTopFaces(canvas, cubes);
        }

        RenderFrontFaces(canvas, cubes);
    }

    void CubesRenderer::RenderSideFaces(
        Canvas& canvas,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        for (const auto& cube : cubes) {
            RenderSingleSideFace(canvas, cube);
        }
    }

    void CubesRenderer::RenderTopFaces(
        Canvas& canvas,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        for (const auto& cube : cubes) {
            RenderSingleTopFace(canvas, cube);
        }
    }

    void CubesRenderer::RenderFrontFaces(
        Canvas& canvas,
        const std::vector<CubeGeometry>& cubes
    ) const
    {
        for (const auto& cube : cubes) {
            RenderSingleFrontFace(canvas, cube);
        }
    }

    void CubesRenderer::RenderSingleSideFace(
        Canvas& canvas,
        const CubeGeometry& cube
    ) const
    {
        const Color sideColor = CalculateSideColor(cube.magnitude);
        const auto points = GetSideFacePoints(cube);

        canvas.DrawPolygon(points, Paint::Fill(sideColor));
    }

    void CubesRenderer::RenderSingleTopFace(
        Canvas& canvas,
        const CubeGeometry& cube
    ) const
    {
        const Color topColor = CalculateTopColor(cube.magnitude);
        const auto points = GetTopFacePoints(cube);

        canvas.DrawPolygon(points, Paint::Fill(topColor));
    }

    void CubesRenderer::RenderSingleFrontFace(
        Canvas& canvas,
        const CubeGeometry& cube
    ) const
    {
        const Color frontColor = CalculateFrontColor(cube.magnitude);

        canvas.DrawRectangle(cube.frontFace, Paint::Fill(frontColor));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color CubesRenderer::CalculateBaseColor(float magnitude) const
    {
        Color color = m_primaryColor;
        color.a = CalculateColorAlpha(magnitude);
        return color;
    }

    Color CubesRenderer::CalculateSideColor(float magnitude) const
    {
        const Color baseColor = CalculateBaseColor(magnitude);
        return Utils::AdjustBrightness(baseColor, m_settings.sideFaceBrightness);
    }

    Color CubesRenderer::CalculateTopColor(float magnitude) const
    {
        const Color baseColor = CalculateBaseColor(magnitude);
        return Utils::AdjustBrightness(baseColor, kTopBrightness);
    }

    Color CubesRenderer::CalculateFrontColor(float magnitude) const
    {
        return CalculateBaseColor(magnitude);
    }

    float CubesRenderer::CalculateColorAlpha(float magnitude) const
    {
        return kAlphaBase + kAlphaRange * magnitude;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool CubesRenderer::IsMagnitudeVisible(float magnitude) const
    {
        return magnitude >= kMinMagnitude;
    }

    bool CubesRenderer::ShouldRenderSideFaces() const
    {
        return m_settings.useSideFace;
    }

    bool CubesRenderer::ShouldRenderTopFaces() const
    {
        return m_settings.useTopFace;
    }

    bool CubesRenderer::AreCubesEmpty(const std::vector<CubeGeometry>& cubes) const
    {
        return cubes.empty();
    }

} // namespace Spectrum