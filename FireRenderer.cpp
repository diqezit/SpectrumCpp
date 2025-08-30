// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FireRenderer.cpp: Implementation of the FireRenderer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "FireRenderer.h"
#include "Utils.h"

namespace Spectrum {

    FireRenderer::FireRenderer()
        : m_gridWidth(0)
        , m_gridHeight(0) {
        UpdateSettings();
        CreateFirePalette();
    }

    void FireRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        InitializeGrid();
    }

    void FireRenderer::UpdateSettings() {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { false, false, 12.0f, 0.93f, 1.2f };
            break;
        case RenderQuality::Medium:
            m_settings = { true, true, 8.0f, 0.95f, 1.5f };
            break;
        case RenderQuality::High:
            m_settings = { true, true, 6.0f, 0.97f, 1.8f };
            break;
        default:
            m_settings = { true, true, 8.0f, 0.95f, 1.5f };
            break;
        }
    }

    void FireRenderer::CreateFirePalette() {
        m_firePalette = {
            Color(0, 0, 0, 0),        // transparent black
            Color(0.2f, 0, 0, 1),     // dark red
            Color(0.5f, 0, 0, 1),     // red
            Color(0.8f, 0.2f, 0, 1),  // orange-red
            Color(1, 0.5f, 0, 1),     // orange
            Color(1, 0.8f, 0, 1),     // yellow-orange
            Color(1, 1, 0.5f, 1),     // bright yellow
            Color(1, 1, 1, 1)         // white
        };
    }

    Color FireRenderer::GetColorFromPalette(float intensity) const {
        if (intensity <= 0.0f) return m_firePalette.front();
        if (intensity >= 1.0f) return m_firePalette.back();

        const float scaled = intensity * (m_firePalette.size() - 1);
        const size_t i1 = static_cast<size_t>(scaled);
        const size_t i2 = std::min(i1 + 1, m_firePalette.size() - 1);
        const float t = scaled - static_cast<float>(i1);

        return Utils::InterpolateColor(
            m_firePalette[i1], m_firePalette[i2], t
        );
    }

    void FireRenderer::InitializeGrid() {
        if (m_width <= 0 || m_height <= 0) {
            m_gridWidth = m_gridHeight = 0;
            m_fireGrid.clear();
            return;
        }

        m_gridWidth = static_cast<int>(m_width / m_settings.pixelSize);
        m_gridHeight = static_cast<int>(m_height / m_settings.pixelSize);

        if (m_gridWidth > 0 && m_gridHeight > 0) {
            m_fireGrid.assign(
                static_cast<size_t>(m_gridWidth * m_gridHeight), 0.0f
            );
        }
        else {
            m_fireGrid.clear();
        }
    }

    void FireRenderer::UpdateAnimation(const SpectrumData& spectrum,
        float deltaTime) {
        (void)deltaTime;
        if (m_gridWidth == 0 || m_gridHeight == 0) return;

        // Decay
        for (float& v : m_fireGrid) {
            v *= m_settings.decay;
        }

        // Heat source at bottom row from spectrum
        const int bottomY = m_gridHeight - 1;
        for (size_t i = 0; i < spectrum.size(); ++i) {
            const int x = static_cast<int>(
                (static_cast<float>(i) /
                    std::max<size_t>(1, spectrum.size() - 1)) *
                (m_gridWidth - 1)
                );
            const int idx = bottomY * m_gridWidth +
                Utils::Clamp(x, 0, m_gridWidth - 1);
            m_fireGrid[idx] = std::max(
                m_fireGrid[idx],
                spectrum[i] * m_settings.heatMultiplier
            );
        }

        // Propagate upwards with optional smoothing and wind
        for (int y = 0; y < m_gridHeight - 1; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                int windOffset = 0;
                if (m_settings.useWind) {
                    windOffset = static_cast<int>(
                        std::sin(GetTime() * 2.0f + x * 0.5f) * 2.0f
                        );
                }
                const int srcX = Utils::Clamp(
                    x - windOffset, 0, m_gridWidth - 1
                );
                const int srcY = y + 1;

                float val = m_fireGrid[srcY * m_gridWidth + srcX];
                if (m_settings.useSmoothing) {
                    const float c = val;
                    const float l = (srcX > 0)
                        ? m_fireGrid[srcY * m_gridWidth + srcX - 1] : c;
                    const float r = (srcX < m_gridWidth - 1)
                        ? m_fireGrid[srcY * m_gridWidth + srcX + 1] : c;
                    val = c * 0.5f + (l + r) * 0.25f;
                }

                m_fireGrid[y * m_gridWidth + x] = val;
            }
        }
    }

    void FireRenderer::DoRender(GraphicsContext& context,
        const SpectrumData& /*spectrum*/) {
        if (m_gridWidth == 0 || m_gridHeight == 0) return;

        for (int y = 0; y < m_gridHeight; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                const float intensity = m_fireGrid[y * m_gridWidth + x];
                if (intensity < 0.01f) continue;

                Color c = GetColorFromPalette(
                    Utils::Clamp(intensity, 0.0f, 1.0f)
                );
                c.a = Utils::SmoothStep(0.0f, 0.8f, intensity);

                context.DrawRectangle(
                    {
                        x * m_settings.pixelSize,
                        y * m_settings.pixelSize,
                        m_settings.pixelSize,
                        m_settings.pixelSize
                    },
                    c
                );
            }
        }
    }

} // namespace Spectrum