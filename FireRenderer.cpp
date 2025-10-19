// FireRenderer.cpp: Implementation of the FireRenderer class.

#include "FireRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor & Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    FireRenderer::FireRenderer()
        : m_gridWidth(0)
        , m_gridHeight(0) {
        UpdateSettings();
        CreateFirePalette();
    }

    void FireRenderer::OnActivate(
        int width,
        int height
    ) {
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
        InitializeGrid();
    }

    void FireRenderer::InitializeGrid() {
        if (m_width <= 0 || m_height <= 0 || m_settings.pixelSize <= 0) {
            m_gridWidth = m_gridHeight = 0;
            m_fireGrid.clear();
            return;
        }

        m_gridWidth = static_cast<int>(m_width / m_settings.pixelSize);
        m_gridHeight = static_cast<int>(m_height / m_settings.pixelSize);

        if (m_gridWidth > 0 && m_gridHeight > 0) {
            m_fireGrid.assign(
                static_cast<size_t>(m_gridWidth) * m_gridHeight,
                0.0f
            );
        }
        else {
            m_fireGrid.clear();
        }
    }

    // define colors from transparent black to bright white for a fire effect
    void FireRenderer::CreateFirePalette() {
        m_firePalette = {
            Color(0.0f, 0.0f, 0.0f, 0.0f),
            Color(0.2f, 0.0f, 0.0f, 1.0f),
            Color(0.5f, 0.0f, 0.0f, 1.0f),
            Color(0.8f, 0.2f, 0.0f, 1.0f),
            Color(1.0f, 0.5f, 0.0f, 1.0f),
            Color(1.0f, 0.8f, 0.0f, 1.0f),
            Color(1.0f, 1.0f, 0.5f, 1.0f),
            Color(1.0f, 1.0f, 1.0f, 1.0f)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation & Fire Simulation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        (void)deltaTime;
        if (m_gridWidth == 0 || m_gridHeight == 0) return;

        ApplyDecay();
        InjectHeat(spectrum);
        PropagateFire();
    }

    void FireRenderer::ApplyDecay() {
        for (float& v : m_fireGrid) {
            v *= m_settings.decay;
        }
    }

    void FireRenderer::InjectHeat(const SpectrumData& spectrum) {
        const int bottomY = m_gridHeight - 1;
        if (bottomY < 0) return;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const int x = static_cast<int>(
                (static_cast<float>(i) / std::max<size_t>(1, spectrum.size() - 1))
                * (m_gridWidth - 1)
                );
            const size_t idx = static_cast<size_t>(bottomY) * m_gridWidth
                + Utils::Clamp(x, 0, m_gridWidth - 1);

            if (idx < m_fireGrid.size()) {
                m_fireGrid[idx] = std::max(
                    m_fireGrid[idx],
                    spectrum[i] * m_settings.heatMultiplier
                );
            }
        }
    }

    void FireRenderer::PropagateFire() {
        std::vector<float> readGrid = m_fireGrid;

        for (int y = 0; y < m_gridHeight - 1; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                float nextValue = GetSourcePixelValue(
                    readGrid, x, y, m_gridWidth, m_settings.useWind, m_time
                );
                nextValue = ApplySmoothing(
                    readGrid, nextValue, x, y, m_gridWidth, m_settings.useSmoothing
                );

                const size_t destIdx = static_cast<size_t>(y) * m_gridWidth + x;
                if (destIdx < m_fireGrid.size()) {
                    m_fireGrid[destIdx] = nextValue;
                }
            }
        }
    }

    float FireRenderer::GetSourcePixelValue(
        const std::vector<float>& readGrid,
        int x,
        int y,
        int gridWidth,
        bool useWind,
        float time
    ) {
        int windOffset = 0;
        if (useWind) {
            windOffset = static_cast<int>(std::sin(time * 2.0f + x * 0.5f) * 2.0f);
        }

        const int srcX = Utils::Clamp(x + windOffset, 0, gridWidth - 1);
        const int srcY = y + 1;

        const size_t srcIdx = static_cast<size_t>(srcY) * gridWidth + srcX;
        if (srcIdx >= readGrid.size()) return 0.0f;

        return readGrid[srcIdx];
    }

    float FireRenderer::ApplySmoothing(
        const std::vector<float>& readGrid,
        float currentValue,
        int x,
        int y,
        int gridWidth,
        bool useSmoothing
    ) {
        if (!useSmoothing) return currentValue;

        const int srcY = y + 1;

        const float l = (x > 0)
            ? readGrid[static_cast<size_t>(srcY) * gridWidth + (x - 1)]
            : currentValue;
        const float r = (x < gridWidth - 1)
            ? readGrid[static_cast<size_t>(srcY) * gridWidth + (x + 1)]
            : currentValue;

        return currentValue * 0.5f + (l + r) * 0.25f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& /*spectrum*/
    ) {
        if (m_gridWidth == 0 || m_gridHeight == 0) return;

        for (int y = 0; y < m_gridHeight; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                RenderPixel(context, x, y);
            }
        }
    }

    void FireRenderer::RenderPixel(
        GraphicsContext& context,
        int x,
        int y
    ) {
        const size_t idx = static_cast<size_t>(y) * m_gridWidth + x;
        if (idx >= m_fireGrid.size()) return;

        const float intensity = m_fireGrid[idx];
        if (intensity < 0.01f) return;

        Color c = GetColorFromPalette(
            Utils::Clamp(intensity, 0.0f, 1.0f)
        );
        c.a *= Utils::SmoothStep(0.0f, 0.1f, intensity);
        if (c.a < 0.01f) return;

        const Rect pixelRect = {
            static_cast<float>(x) * m_settings.pixelSize,
            static_cast<float>(y) * m_settings.pixelSize,
            m_settings.pixelSize,
            m_settings.pixelSize
        };
        context.DrawRectangle(pixelRect, c);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color FireRenderer::GetColorFromPalette(float intensity) const {
        if (intensity <= 0.0f) return m_firePalette.front();
        if (intensity >= 1.0f) return m_firePalette.back();

        const float scaled = intensity * (m_firePalette.size() - 1);
        const size_t i1 = static_cast<size_t>(scaled);
        const size_t i2 = std::min(i1 + 1, m_firePalette.size() - 1);
        const float t = scaled - static_cast<float>(i1);

        return Utils::InterpolateColor(
            m_firePalette[i1],
            m_firePalette[i2],
            t
        );
    }
}