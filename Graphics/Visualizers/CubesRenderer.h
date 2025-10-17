#ifndef SPECTRUM_CPP_CUBES_RENDERER_H
#define SPECTRUM_CPP_CUBES_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the CubesRenderer for 3D-style bar visualization.
//
// This renderer creates a pseudo-3D effect by rendering bars as cubes with
// visible front, top, and side faces. The perspective effect is achieved
// through polygon rendering with appropriate shading.
//
// Key features:
// - Three visible faces per cube (front, top, side)
// - Quality-dependent shadows and face visibility
// - Brightness variation for depth perception
// - Batch rendering optimization for front faces
//
// Design notes:
// - All rendering methods are const (stateless rendering)
// - Correct rendering order: shadows -> sides -> tops -> fronts
// - Face brightness adjusted for 3D effect (top lighter, side darker)
// - CubeGeometry struct pre-calculates all face dimensions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class CubesRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        CubesRenderer();
        ~CubesRenderer() override = default;

        CubesRenderer(const CubesRenderer&) = delete;
        CubesRenderer& operator=(const CubesRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::Cubes; }
        [[nodiscard]] std::string_view GetName() const override { return "Cubes"; }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BaseRenderer Overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSettings() override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Settings
        {
            bool useTopFace;
            bool useSideFace;
            bool useShadow;
            float topHeightRatio;
            float sideFaceBrightness;
            float perspective;
        };

        struct CubeGeometry
        {
            Rect frontFace{};
            float topHeight = 0.0f;
            float sideWidth = 0.0f;
            float magnitude = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Cube Collection
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void CollectVisibleCubes(
            std::vector<CubeGeometry>& cubes,
            const SpectrumData& spectrum,
            const RenderUtils::BarLayout& layout
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] CubeGeometry CalculateCubeGeometry(
            size_t index,
            float magnitude,
            const RenderUtils::BarLayout& layout
        ) const;

        [[nodiscard]] Rect CalculateFrontFace(
            size_t index,
            float height,
            const RenderUtils::BarLayout& layout
        ) const;

        [[nodiscard]] float CalculateBarHeight(float magnitude) const;
        [[nodiscard]] float CalculateTopHeight(float barWidth) const;
        [[nodiscard]] float CalculateSideWidth(float barWidth) const;

        [[nodiscard]] std::vector<Point> GetSideFacePoints(
            const CubeGeometry& cube
        ) const;

        [[nodiscard]] std::vector<Point> GetTopFacePoints(
            const CubeGeometry& cube
        ) const;

        [[nodiscard]] Point CalculateSidePoint1(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateSidePoint2(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateSidePoint3(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateSidePoint4(const CubeGeometry& cube) const;

        [[nodiscard]] Point CalculateTopPoint1(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateTopPoint2(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateTopPoint3(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateTopPoint4(const CubeGeometry& cube) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Face Rendering (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderCubes(
            Canvas& canvas,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderCubesWithShadows(
            Canvas& canvas,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderAllFaces(
            Canvas& canvas,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderSideFaces(
            Canvas& canvas,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderTopFaces(
            Canvas& canvas,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderFrontFaces(
            Canvas& canvas,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderSingleSideFace(
            Canvas& canvas,
            const CubeGeometry& cube
        ) const;

        void RenderSingleTopFace(
            Canvas& canvas,
            const CubeGeometry& cube
        ) const;

        void RenderSingleFrontFace(
            Canvas& canvas,
            const CubeGeometry& cube
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color CalculateBaseColor(float magnitude) const;
        [[nodiscard]] Color CalculateSideColor(float magnitude) const;
        [[nodiscard]] Color CalculateTopColor(float magnitude) const;
        [[nodiscard]] Color CalculateFrontColor(float magnitude) const;

        [[nodiscard]] float CalculateColorAlpha(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsMagnitudeVisible(float magnitude) const;
        [[nodiscard]] bool ShouldRenderSideFaces() const;
        [[nodiscard]] bool ShouldRenderTopFaces() const;
        [[nodiscard]] bool AreCubesEmpty(const std::vector<CubeGeometry>& cubes) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CUBES_RENDERER_H