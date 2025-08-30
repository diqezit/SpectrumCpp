// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// WaveRenderer.cpp: Implementation of the WaveRenderer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WaveRenderer.h"
#include "Utils.h"

namespace Spectrum {

    WaveRenderer::WaveRenderer() {
        m_primaryColor = Color::FromRGB(100, 255, 100);
        UpdateSettings();
    }

    void WaveRenderer::UpdateSettings() {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { 1.5f, false, 0.0f };
            break;
        case RenderQuality::Medium:
            m_settings = { 2.0f, true, 0.6f };
            break;
        case RenderQuality::High:
            m_settings = { 3.0f, true, 0.7f };
            break;
        default:
            m_settings = { 2.0f, true, 0.6f };
            break;
        }
    }

    void WaveRenderer::DoRender(GraphicsContext& context,
        const SpectrumData& spectrum) {
        BuildPolylineFromSpectrum(spectrum, 0.5f, 0.4f, m_points);
        context.DrawPolyline(m_points, m_primaryColor, m_settings.lineWidth);

        if (!m_settings.useReflection) return;

        std::vector<Point> refl = m_points;
        for (auto& p : refl) {
            p.y = static_cast<float>(m_height) - p.y;
        }

        Color rc = m_primaryColor;
        rc.a *= m_settings.reflectionStrength;

        context.DrawPolyline(refl, rc, m_settings.lineWidth * 0.8f);
    }

} // namespace Spectrum