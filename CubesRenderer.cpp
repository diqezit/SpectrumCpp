// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CubesRenderer.cpp: Implementation of the CubesRenderer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "CubesRenderer.h"
#include "Utils.h"

namespace Spectrum {

    CubesRenderer::CubesRenderer() {
        m_primaryColor = Color::FromRGB(200, 100, 255);
        UpdateSettings();
    }

    void CubesRenderer::UpdateSettings() {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { false, true, false, 0.2f, 0.7f, 0.15f };
            break;
        case RenderQuality::Medium:
            m_settings = { true, true, true, 0.25f, 0.6f, 0.25f };
            break;
        case RenderQuality::High:
            m_settings = { true, true, true, 0.3f, 0.5f, 0.35f };
            break;
        default:
            m_settings = { true, true, true, 0.25f, 0.6f, 0.25f };
            break;
        }
    }

    void CubesRenderer::DoRender(GraphicsContext& context,
        const SpectrumData& spectrum) {
        const size_t n = spectrum.size();
        const float spacing = 2.0f;
        const auto bl = ComputeBarLayout(n, spacing);
        if (bl.barWidth <= 0.0f) return;

        std::vector<CubeData> cubes;
        cubes.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const float mag = spectrum[i];
            if (mag < 0.01f) continue;

            const float h = MagnitudeToHeight(mag, 0.9f);
            CubeData cd;
            cd.frontFace = {
                i * bl.totalBarWidth,
                static_cast<float>(m_height) - h,
                bl.barWidth,
                h
            };
            cd.topHeight = bl.barWidth * m_settings.topHeightRatio;
            cd.sideWidth = bl.barWidth * m_settings.perspective;
            cd.magnitude = mag;
            cubes.push_back(cd);
        }

        for (const auto& cube : cubes) {
            Color front = m_primaryColor;
            front.a = 0.6f + 0.4f * cube.magnitude;

            if (m_settings.useShadow) {
                context.DrawRectangle(
                    { cube.frontFace.x + 3, cube.frontFace.y + 3,
                      cube.frontFace.width, cube.frontFace.height },
                    { 0, 0, 0, 0.2f }
                );
            }

            if (m_settings.useSideFace) {
                Color side = Utils::AdjustBrightness(
                    front, m_settings.sideFaceBrightness
                );
                Point p1 = { cube.frontFace.GetRight(), cube.frontFace.y };
                Point p2 = { p1.x + cube.sideWidth,
                            p1.y - cube.topHeight };
                Point p3 = { p2.x, p2.y + cube.frontFace.height };
                Point p4 = { p1.x, cube.frontFace.GetBottom() };
                context.DrawPolygon({ p1, p2, p3, p4 }, side);
            }

            if (m_settings.useTopFace) {
                Color top = Utils::AdjustBrightness(front, 1.2f);
                Point p1 = { cube.frontFace.x, cube.frontFace.y };
                Point p2 = { cube.frontFace.GetRight(), cube.frontFace.y };
                Point p3 = { p2.x + cube.sideWidth,
                            p2.y - cube.topHeight };
                Point p4 = { p1.x + cube.sideWidth,
                            p1.y - cube.topHeight };
                context.DrawPolygon({ p1, p2, p3, p4 }, top);
            }

            context.DrawRectangle(cube.frontFace, front);
        }
    }

} // namespace Spectrum