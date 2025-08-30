// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CircularWaveRenderer.cpp: Implementation of CircularWaveRenderer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "CircularWaveRenderer.h"
#include "Utils.h"

namespace Spectrum {

    CircularWaveRenderer::CircularWaveRenderer()
        : m_angle(0.0f)
        , m_waveTime(0.0f) {
        m_primaryColor = Color::FromRGB(0, 150, 255);
        UpdateSettings();
    }

    void CircularWaveRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        m_circlePoints.clear();
    }

    void CircularWaveRenderer::UpdateSettings() {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { 32, false, 6.0f, 16, 0.5f, 2.0f };
            break;
        case RenderQuality::Medium:
            m_settings = { 64, true, 7.0f, 24, 0.5f, 2.0f };
            break;
        case RenderQuality::High:
            m_settings = { 128, true, 8.0f, 32, 0.5f, 2.0f };
            break;
        default:
            m_settings = { 64, true, 7.0f, 24, 0.5f, 2.0f };
            break;
        }
        m_circlePoints.clear();
    }

    void CircularWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum, float deltaTime) {
        const float avg = GetAverageMagnitude(spectrum);
        const float rotFactor = 0.3f;

        m_angle += m_settings.rotationSpeed *
            (1.0f + avg * rotFactor) * deltaTime;
        if (m_angle > TWO_PI) m_angle -= TWO_PI;

        m_waveTime += m_settings.waveSpeed * deltaTime;
    }

    void CircularWaveRenderer::PrecomputeCirclePoints() {
        if (!m_circlePoints.empty()) return;

        m_circlePoints.reserve(m_settings.pointsPerCircle + 1);
        const float step = TWO_PI / m_settings.pointsPerCircle;

        for (int i = 0; i <= m_settings.pointsPerCircle; ++i) {
            float a = i * step;
            m_circlePoints.emplace_back(std::cos(a), std::sin(a));
        }
    }

    void CircularWaveRenderer::DoRender(GraphicsContext& context,
        const SpectrumData& spectrum) {
        PrecomputeCirclePoints();

        const Point center = { m_width * 0.5f, m_height * 0.5f };
        const float maxR = std::min(m_width, m_height) * 0.45f;
        const float r0 = 30.0f;

        const int rings = std::min<int>(
            static_cast<int>(spectrum.size()),
            m_settings.maxRings
        );
        const float stepR = (maxR - r0) / std::max(1, rings);

        for (int i = rings - 1; i >= 0; --i) {
            const float mag = SegmentAverage(
                spectrum, rings, static_cast<size_t>(i)
            );
            if (mag < 0.01f) continue;

            const float phase = 0.1f;
            const float baseR = r0 + i * stepR;
            const float wobble = std::sin(m_waveTime + i * phase + m_angle)
                * mag * stepR;
            const float radius = baseR + wobble;
            if (radius <= 0.0f) continue;

            const float distK = 1.0f - (radius / maxR);
            const float alpha = Utils::Clamp(mag * 1.5f * distK, 0.0f, 1.0f);
            float stroke = Utils::Clamp(
                1.5f + mag * 6.0f, 1.5f, m_settings.maxStroke
            );

            Color c = m_primaryColor;
            c.a = alpha;

            std::vector<Point> path;
            path.reserve(m_circlePoints.size());
            for (const auto& p : m_circlePoints) {
                path.push_back(center + p * radius);
            }

            context.DrawPolyline(path, c, stroke);

            if (m_settings.useGlow && mag > 0.5f) {
                Color glow = m_primaryColor;
                glow.a = alpha * 0.5f;
                context.DrawPolyline(path, glow, stroke * 2.0f);
            }
        }
    }

} // namespace Spectrum