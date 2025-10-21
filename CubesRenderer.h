// CubesRenderer.h
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

#include "BaseRenderer.h"
#include "RenderUtils.h"

namespace Spectrum {

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
            GraphicsContext& context,
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
            Rect frontFace;
            float topHeight;
            float sideWidth;
            float magnitude;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] CubeGeometry CalculateCubeGeometry(
            size_t index,
            float magnitude,
            const RenderUtils::BarLayout& layout
        ) const;

        [[nodiscard]] std::vector<Point> GetSideFacePoints(
            const CubeGeometry& cube
        ) const;

        [[nodiscard]] std::vector<Point> GetTopFacePoints(
            const CubeGeometry& cube
        ) const;

        [[nodiscard]] Color GetCubeColor(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderCubes(
            GraphicsContext& context,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderCubesWithShadows(
            GraphicsContext& context,
            const std::vector<CubeGeometry>& cubes
        ) const;

        void RenderSideFace(
            GraphicsContext& context,
            const CubeGeometry& cube
        ) const;

        void RenderTopFace(
            GraphicsContext& context,
            const CubeGeometry& cube
        ) const;

        void RenderFrontFaces(
            GraphicsContext& context,
            const std::vector<CubeGeometry>& cubes
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
    };

} // namespace Spectrum

#endif