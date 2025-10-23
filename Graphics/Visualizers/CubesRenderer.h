#ifndef SPECTRUM_CPP_CUBES_RENDERER_H
#define SPECTRUM_CPP_CUBES_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the CubesRenderer for 3D-style bar visualization.
//
// This renderer displays spectrum data as pseudo-3D cubes with visible
// front, top, and side faces. Creates an isometric-style effect through
// perspective projection and face shading.
//
// Key features:
// - Three visible faces per cube (front, top, side)
// - Configurable perspective ratio for 3D depth effect
// - Independent brightness control per face for realistic lighting
// - Optional shadow rendering with configurable offset
// - Uses GeometryHelpers for all geometric calculations
//
// Design notes:
// - All rendering methods are const (stateless rendering)
// - Face visibility controlled by quality settings
// - Brightness decreases from front to top to side for depth
// - Shadow renders once for all cubes in batch
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
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
        // Settings & Data Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        using Settings = Settings::CubesSettings;

        struct CubeGeometry
        {
            Rect frontFace{};
            float topHeight = 0.0f;
            float sideWidth = 0.0f;
            float magnitude = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void CollectVisibleCubes(
            std::vector<CubeGeometry>& cubes,
            const SpectrumData& spectrum,
            const RenderUtils::BarLayout& layout
        ) const;

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

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Face Point Generation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<Point> GetSideFacePoints(const CubeGeometry& cube) const;
        [[nodiscard]] std::vector<Point> GetTopFacePoints(const CubeGeometry& cube) const;

        [[nodiscard]] Point CalculateSidePoint1(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateSidePoint2(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateSidePoint3(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateSidePoint4(const CubeGeometry& cube) const;

        [[nodiscard]] Point CalculateTopPoint1(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateTopPoint2(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateTopPoint3(const CubeGeometry& cube) const;
        [[nodiscard]] Point CalculateTopPoint4(const CubeGeometry& cube) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderCubes(Canvas& canvas, const std::vector<CubeGeometry>& cubes) const;
        void RenderCubesWithShadows(Canvas& canvas, const std::vector<CubeGeometry>& cubes) const;
        void RenderAllFaces(Canvas& canvas, const std::vector<CubeGeometry>& cubes) const;
        void RenderSideFaces(Canvas& canvas, const std::vector<CubeGeometry>& cubes) const;
        void RenderTopFaces(Canvas& canvas, const std::vector<CubeGeometry>& cubes) const;
        void RenderFrontFaces(Canvas& canvas, const std::vector<CubeGeometry>& cubes) const;

        void RenderSingleSideFace(Canvas& canvas, const CubeGeometry& cube) const;
        void RenderSingleTopFace(Canvas& canvas, const CubeGeometry& cube) const;
        void RenderSingleFrontFace(Canvas& canvas, const CubeGeometry& cube) const;

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