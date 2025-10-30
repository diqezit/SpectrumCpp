#ifndef SPECTRUM_CPP_CUBES_RENDERER_H
#define SPECTRUM_CPP_CUBES_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class CubesRenderer final : public BaseRenderer<CubesRenderer>
    {
    public:
        CubesRenderer();
        ~CubesRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Cubes;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Cubes";
        }

    protected:
        void UpdateSettings() override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        using Settings = Settings::CubesSettings;

        static constexpr float kMinMagnitude = 0.01f;
        static constexpr float kSpacing = 2.0f;
        static constexpr float kHeightScale = 0.9f;
        static constexpr float kTopBrightness = 1.2f;
        static constexpr float kAlphaBase = 0.6f;
        static constexpr float kAlphaRange = 0.4f;
        static constexpr float kShadowOffsetX = 2.0f;
        static constexpr float kShadowOffsetY = 2.0f;
        static constexpr float kShadowAlpha = 0.3f;

        struct CubeData {
            Rect frontFace;
            float topHeight;
            float sideWidth;
            float magnitude;
            Color baseColor;
            Color sideColor;
            Color topColor;
        };

        [[nodiscard]] std::vector<CubeData> CollectVisibleCubes(
            const SpectrumData& spectrum,
            const BarLayout& layout
        ) const;

        void RenderCubeShadows(
            Canvas& canvas,
            const std::vector<CubeData>& cubes
        ) const;

        void RenderCubeSides(
            Canvas& canvas,
            const std::vector<CubeData>& cubes
        ) const;

        void RenderCubeTops(
            Canvas& canvas,
            const std::vector<CubeData>& cubes
        ) const;

        void RenderCubeFronts(
            Canvas& canvas,
            const std::vector<CubeData>& cubes
        ) const;

        [[nodiscard]] CubeData CreateCubeData(
            size_t index,
            float magnitude,
            const BarLayout& layout
        ) const;

        [[nodiscard]] std::vector<Point> GetSideFacePoints(
            const CubeData& cube
        ) const;

        [[nodiscard]] std::vector<Point> GetTopFacePoints(
            const CubeData& cube
        ) const;

        [[nodiscard]] Color CalculateBaseColor(
            float magnitude
        ) const;

        Settings m_settings;
    };

} // namespace Spectrum

#endif