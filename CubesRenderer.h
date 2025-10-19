// CubesRenderer.h

#ifndef SPECTRUM_CPP_CUBES_RENDERER_H
#define SPECTRUM_CPP_CUBES_RENDERER_H

#include "BaseRenderer.h"
#include "RenderUtils.h"

namespace Spectrum {

    class CubesRenderer final : public BaseRenderer {
    public:
        CubesRenderer();
        ~CubesRenderer() override = default;

        RenderStyle GetStyle() const override {
            return RenderStyle::Cubes;
        }

        std::string_view GetName() const override {
            return "Cubes";
        }

    protected:
        void UpdateSettings() override;

        void DoRender(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Settings {
            bool useTopFace;
            bool useSideFace;
            bool useShadow;
            float topHeightRatio;
            float sideFaceBrightness;
            float perspective;
        };

        struct CubeGeometry {
            Rect frontFace;
            float topHeight;
            float sideWidth;
            float magnitude;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helper Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        CubeGeometry CalculateCubeGeometry(
            size_t index,
            float magnitude,
            const RenderUtils::BarLayout& layout
        ) const;

        std::vector<Point> GetSideFacePoints(const CubeGeometry& cube) const;
        std::vector<Point> GetTopFacePoints(const CubeGeometry& cube) const;

        Color GetCubeColor(float magnitude) const;

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
        // Member Data
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CUBES_RENDERER_H