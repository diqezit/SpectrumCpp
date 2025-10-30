#include "Graphics/Visualizers/CubesRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Geometry;

    CubesRenderer::CubesRenderer() {
        UpdateSettings();
    }

    void CubesRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
    }

    void CubesRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        const auto layout = CalculateBarLayout(spectrum.size(), kSpacing);
        if (layout.barWidth <= 0.0f) return;

        const auto cubes = CollectVisibleCubes(spectrum, layout);
        if (cubes.empty()) return;

        if (m_settings.useShadow) {
            RenderCubeShadows(canvas, cubes);
        }

        if (m_settings.useSideFace) {
            RenderCubeSides(canvas, cubes);
        }

        if (m_settings.useTopFace) {
            RenderCubeTops(canvas, cubes);
        }

        RenderCubeFronts(canvas, cubes);
    }

    std::vector<CubesRenderer::CubeData> CubesRenderer::CollectVisibleCubes(
        const SpectrumData& spectrum,
        const BarLayout& layout
    ) const {
        std::vector<CubeData> cubes;
        cubes.reserve(spectrum.size());

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);

            if (magnitude >= kMinMagnitude) {
                cubes.push_back(CreateCubeData(i, magnitude, layout));
            }
        }

        return cubes;
    }

    void CubesRenderer::RenderCubeShadows(
        Canvas& canvas,
        const std::vector<CubeData>& cubes
    ) const {
        const Color shadowColor = AdjustAlpha(
            Color::Black(),
            kShadowAlpha
        );
        const Paint shadowPaint = Paint::Fill(shadowColor);

        for (const auto& cube : cubes) {
            Rect shadowRect = cube.frontFace;
            shadowRect.x += kShadowOffsetX;
            shadowRect.y += kShadowOffsetY;

            canvas.DrawRectangle(shadowRect, shadowPaint);

            if (m_settings.useSideFace) {
                auto sidePoints = GetSideFacePoints(cube);
                for (auto& point : sidePoints) {
                    point.x += kShadowOffsetX;
                    point.y += kShadowOffsetY;
                }
                canvas.DrawPolygon(sidePoints, shadowPaint);
            }

            if (m_settings.useTopFace) {
                auto topPoints = GetTopFacePoints(cube);
                for (auto& point : topPoints) {
                    point.x += kShadowOffsetX;
                    point.y += kShadowOffsetY;
                }
                canvas.DrawPolygon(topPoints, shadowPaint);
            }
        }
    }

    void CubesRenderer::RenderCubeSides(
        Canvas& canvas,
        const std::vector<CubeData>& cubes
    ) const {
        for (const auto& cube : cubes) {
            canvas.DrawPolygon(
                GetSideFacePoints(cube),
                Paint::Fill(cube.sideColor)
            );
        }
    }

    void CubesRenderer::RenderCubeTops(
        Canvas& canvas,
        const std::vector<CubeData>& cubes
    ) const {
        for (const auto& cube : cubes) {
            canvas.DrawPolygon(
                GetTopFacePoints(cube),
                Paint::Fill(cube.topColor)
            );
        }
    }

    void CubesRenderer::RenderCubeFronts(
        Canvas& canvas,
        const std::vector<CubeData>& cubes
    ) const {
        RectBatch frontBatches;

        for (const auto& cube : cubes) {
            frontBatches[cube.baseColor].push_back(cube.frontFace);
        }

        for (const auto& [color, rects] : frontBatches) {
            if (!rects.empty()) {
                canvas.DrawRectangleBatch(rects, Paint::Fill(color));
            }
        }
    }

    CubesRenderer::CubeData CubesRenderer::CreateCubeData(
        size_t index,
        float magnitude,
        const BarLayout& layout
    ) const {
        const float height = RenderUtils::MagnitudeToHeight(
            magnitude,
            GetHeight(),
            kHeightScale
        );

        CubeData cube;
        cube.frontFace = GetBarRect(layout, index, height);
        cube.topHeight = layout.barWidth * m_settings.topHeightRatio;
        cube.sideWidth = layout.barWidth * m_settings.perspective;
        cube.magnitude = magnitude;
        cube.baseColor = CalculateBaseColor(magnitude);
        cube.sideColor = AdjustBrightness(
            cube.baseColor,
            m_settings.sideFaceBrightness
        );
        cube.topColor = AdjustBrightness(
            cube.baseColor,
            kTopBrightness
        );

        return cube;
    }

    std::vector<Point> CubesRenderer::GetSideFacePoints(
        const CubeData& cube
    ) const {
        const Point topRight = GetTopRight(cube.frontFace);
        const Point bottomRight = GetBottomRight(cube.frontFace);

        return {
            topRight,
            Add(topRight, {cube.sideWidth, -cube.topHeight}),
            Add(bottomRight, {cube.sideWidth, -cube.topHeight}),
            bottomRight
        };
    }

    std::vector<Point> CubesRenderer::GetTopFacePoints(
        const CubeData& cube
    ) const {
        const Point topLeft = GetTopLeft(cube.frontFace);
        const Point topRight = GetTopRight(cube.frontFace);

        return {
            topLeft,
            topRight,
            Add(topRight, {cube.sideWidth, -cube.topHeight}),
            Add(topLeft, {cube.sideWidth, -cube.topHeight})
        };
    }

    Color CubesRenderer::CalculateBaseColor(
        float magnitude
    ) const {
        const float alpha = kAlphaBase + kAlphaRange * magnitude;
        return AdjustAlpha(GetPrimaryColor(), alpha);
    }

} // namespace Spectrum