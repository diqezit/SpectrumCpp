// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CubesRenderer.h: Renders the spectrum as pseudo-3D cubes.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_CUBES_RENDERER_H
#define SPECTRUM_CPP_CUBES_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class CubesRenderer final : public BaseRenderer {
    public:
        CubesRenderer();
        ~CubesRenderer() override = default;

        RenderStyle GetStyle() const override { return RenderStyle::Cubes; }
        std::string_view GetName() const override { return "Cubes"; }

    protected:
        void UpdateSettings() override;
        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        struct Settings {
            bool useTopFace;
            bool useSideFace;
            bool useShadow;
            float topHeightRatio;
            float sideFaceBrightness;
            float perspective;
        };

        struct CubeData {
            Rect frontFace;
            float topHeight;
            float sideWidth;
            float magnitude;
        };

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CUBES_RENDERER_H