// FireRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the FireRenderer for pixelated fire effect visualization.
//
// Implementation details:
// - Fire simulation uses 2-buffer propagation (read from old, write to new)
// - Heat decay applied each frame (exponential cooling)
// - Spectrum data injected at bottom row as heat sources
// - Wind effect: horizontal sine wave offset based on time
// - Smoothing: neighbor averaging for softer flames
// - Color mapped from intensity via interpolated palette
//
// Performance considerations:
// - Grid size controlled by quality (pixel size)
// - Skip rendering pixels below visibility threshold
// - Early exit for invalid grid or empty spectrum
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "FireRenderer.h"
#include "../API/D2DHelpers.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "../API/Canvas.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr float kWindSpeed = 2.0f;
        constexpr float kWindPhaseOffset = 0.5f;
        constexpr float kWindAmplitude = 2.0f;
        constexpr float kSmoothingCenter = 0.5f;
        constexpr float kSmoothingSide = 0.25f;
        constexpr float kMinVisibleIntensity = 0.01f;
        constexpr float kMinVisibleAlpha = 0.01f;
        constexpr float kAlphaSmoothStepMin = 0.0f;
        constexpr float kAlphaSmoothStepMax = 0.1f;
    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    FireRenderer::FireRenderer()
        : m_gridWidth(0)
        , m_gridHeight(0)
    {
        UpdateSettings();
        CreateFirePalette();
    }

    void FireRenderer::OnActivate(int width, int height)
    {
        BaseRenderer::OnActivate(width, height);
        InitializeGrid();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_settings = { false, false, 12.0f, 0.93f, 1.2f };
            break;
        case RenderQuality::High:
            m_settings = { true, true, 6.0f, 0.97f, 1.8f };
            break;
        case RenderQuality::Medium:
        default:
            m_settings = { true, true, 8.0f, 0.95f, 1.5f };
            break;
        }
        InitializeGrid();
    }

    void FireRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float /*deltaTime*/
    )
    {
        if (m_gridWidth == 0 || m_gridHeight == 0) return;

        ApplyDecay();
        InjectHeat(spectrum);
        PropagateFire();
    }

    void FireRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        if (m_gridWidth == 0 || m_gridHeight == 0) return;

        for (int y = 0; y < m_gridHeight; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                RenderPixel(canvas, x, y);
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::InitializeGrid()
    {
        if (m_width <= 0 || m_height <= 0 || m_settings.pixelSize <= 0.0f) {
            m_gridWidth = 0;
            m_gridHeight = 0;
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

    void FireRenderer::CreateFirePalette()
    {
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
    // Fire Simulation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::ApplyDecay()
    {
        for (float& value : m_fireGrid) {
            value *= m_settings.decay;
        }
    }

    void FireRenderer::InjectHeat(const SpectrumData& spectrum)
    {
        const int bottomY = m_gridHeight - 1;
        if (bottomY < 0) return;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float normalized = NormalizedFloat(spectrum[i]);
            const int x = static_cast<int>(
                (static_cast<float>(i) / std::max<size_t>(1, spectrum.size() - 1))
                * (m_gridWidth - 1)
                );

            const size_t idx = static_cast<size_t>(bottomY) * m_gridWidth
                + Utils::Clamp(x, 0, m_gridWidth - 1);

            if (idx < m_fireGrid.size()) {
                m_fireGrid[idx] = std::max(
                    m_fireGrid[idx],
                    normalized * m_settings.heatMultiplier
                );
            }
        }
    }

    void FireRenderer::PropagateFire()
    {
        const std::vector<float> readGrid = m_fireGrid;

        for (int y = 0; y < m_gridHeight - 1; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                float nextValue = GetSourcePixelValue(
                    readGrid,
                    x,
                    y,
                    m_gridWidth,
                    m_settings.useWind,
                    GetTime()
                );

                nextValue = ApplySmoothing(
                    readGrid,
                    nextValue,
                    x,
                    y,
                    m_gridWidth,
                    m_settings.useSmoothing
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
    )
    {
        int windOffset = 0;
        if (useWind) {
            windOffset = static_cast<int>(
                std::sin(time * kWindSpeed + x * kWindPhaseOffset) * kWindAmplitude
                );
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
    )
    {
        if (!useSmoothing) return currentValue;

        const int srcY = y + 1;

        const float leftValue = (x > 0)
            ? readGrid[static_cast<size_t>(srcY) * gridWidth + (x - 1)]
            : currentValue;

        const float rightValue = (x < gridWidth - 1)
            ? readGrid[static_cast<size_t>(srcY) * gridWidth + (x + 1)]
            : currentValue;

        return currentValue * kSmoothingCenter + (leftValue + rightValue) * kSmoothingSide;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::RenderPixel(
        Canvas& canvas,
        int x,
        int y
    ) const
    {
        const size_t idx = static_cast<size_t>(y) * m_gridWidth + x;
        if (idx >= m_fireGrid.size()) return;

        const float intensity = m_fireGrid[idx];
        if (intensity < kMinVisibleIntensity) return;

        Color color = GetColorFromPalette(Utils::Clamp(intensity, 0.0f, 1.0f));
        color.a *= Utils::SmoothStep(kAlphaSmoothStepMin, kAlphaSmoothStepMax, intensity);

        if (color.a < kMinVisibleAlpha) return;

        const Rect pixelRect{
            static_cast<float>(x) * m_settings.pixelSize,
            static_cast<float>(y) * m_settings.pixelSize,
            m_settings.pixelSize,
            m_settings.pixelSize
        };

        canvas.DrawRectangle(pixelRect, Paint{ color, true });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color FireRenderer::GetColorFromPalette(float intensity) const
    {
        if (intensity <= 0.0f) return m_firePalette.front();
        if (intensity >= 1.0f) return m_firePalette.back();

        const float scaled = intensity * (m_firePalette.size() - 1);
        const size_t i1 = static_cast<size_t>(scaled);
        const size_t i2 = std::min(i1 + 1, m_firePalette.size() - 1);
        const float t = scaled - static_cast<float>(i1);

        return Utils::InterpolateColor(m_firePalette[i1], m_firePalette[i2], t);
    }

} // namespace Spectrum