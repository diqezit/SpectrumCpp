#ifndef SPECTRUM_CPP_CUBES_RENDERER_H
#define SPECTRUM_CPP_CUBES_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CubesRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class CubesRenderer final : public BaseRenderer<CubesRenderer> {
    public:
        CubesRenderer() { UpdateSettings(); }
        [[nodiscard]] std::string_view GetName() const override { return "Cubes"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::CubesSettings>();
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            const auto layout = CalculateBarLayout(spectrum.size(), kSpacing);
            if (layout.barWidth <= 0.0f) return;

            std::vector<Cube> cubes;
            cubes.reserve(spectrum.size());
            for (size_t i = 0; i < spectrum.size(); ++i) {
                const float mag = Helpers::Sanitize::Normalized(spectrum[i]);
                if (mag >= kMinMagnitude)
                    cubes.push_back(MakeCube(i, mag, layout));
            }
            if (cubes.empty()) return;

            if (m_settings.useShadow) DrawShadows(ctx, cubes);

            if (m_settings.useSideFace) {
                for (const auto& cube : cubes)
                    Draw::FillPolygon(ctx, SidePoints(cube), cube.side);
            }
            if (m_settings.useTopFace) {
                for (const auto& cube : cubes)
                    Draw::FillPolygon(ctx, TopPoints(cube), cube.top);
            }

            RectBatch fronts;
            for (const auto& cube : cubes)
                fronts[cube.frontColor].push_back(cube.front);
            RenderRectBatches(ctx, fronts);
        }

    private:
        static constexpr float kMinMagnitude = 0.01f;
        static constexpr float kSpacing = 2.0f;
        static constexpr float kHeightScale = 0.9f;
        static constexpr float kTopBrightness = 1.2f;
        static constexpr float kAlphaBase = 0.6f;
        static constexpr float kAlphaRange = 0.4f;

        struct Cube {
            Rect front;
            float topH = 0.0f;
            float sideW = 0.0f;
            Color frontColor;
            Color side;
            Color top;
        };

        [[nodiscard]] Cube MakeCube(size_t index, float mag, const BarLayout& layout) const {
            const Color base = AdjustAlpha(GetPrimaryColor(), kAlphaBase + kAlphaRange * mag);
            return {
                GetBarRect(layout, index, RenderUtils::MagnitudeToHeight(mag, GetHeight(), kHeightScale)),
                layout.barWidth * m_settings.topHeightRatio,
                layout.barWidth * m_settings.perspective,
                base,
                AdjustBrightness(base, m_settings.sideFaceBrightness),
                AdjustBrightness(base, kTopBrightness)
            };
        }

        [[nodiscard]] std::vector<Point> SidePoints(const Cube& cube) const {
            using namespace Helpers::Geometry;
            const Point tr = GetTopRight(cube.front);
            const Point br = GetBottomRight(cube.front);
            return { tr, Add(tr, { cube.sideW, -cube.topH }), Add(br, { cube.sideW, -cube.topH }), br };
        }

        [[nodiscard]] std::vector<Point> TopPoints(const Cube& cube) const {
            using namespace Helpers::Geometry;
            const Point tl = GetTopLeft(cube.front);
            const Point tr = GetTopRight(cube.front);
            return { tl, tr, Add(tr, { cube.sideW, -cube.topH }), Add(tl, { cube.sideW, -cube.topH }) };
        }

        void DrawShadows(BLContext& ctx, const std::vector<Cube>& cubes) const {
            const Color shadow = RenderUtils::ShadowColor();
            for (const auto& cube : cubes) {
                Draw::FillRect(ctx, RenderUtils::OffsetRect(cube.front), shadow);
                if (m_settings.useSideFace) {
                    auto pts = SidePoints(cube);
                    Offset(pts);
                    Draw::FillPolygon(ctx, pts, shadow);
                }
                if (m_settings.useTopFace) {
                    auto pts = TopPoints(cube);
                    Offset(pts);
                    Draw::FillPolygon(ctx, pts, shadow);
                }
            }
        }

        static void Offset(std::vector<Point>& pts) {
            for (auto& p : pts) {
                p.x += RenderUtils::kShadowOffset;
                p.y += RenderUtils::kShadowOffset;
            }
        }

        Settings::CubesSettings m_settings{};
    };

} // namespace Spectrum

#endif