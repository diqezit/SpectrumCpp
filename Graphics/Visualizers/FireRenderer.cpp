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
// - Uses GeometryHelpers for all geometric operations
//
// Performance considerations:
// - Grid size controlled by quality (pixel size)
// - Skip rendering pixels below visibility threshold
// - Early exit for invalid grid or empty spectrum
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/FireRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

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

        constexpr size_t kPaletteSize = 8;

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

    void FireRenderer::OnActivate(
        int width,
        int height
    )
    {
        BaseRenderer::OnActivate(width, height);
        InitializeGrid();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void FireRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<FireRenderer>(m_quality);

        InitializeGrid();
    }

    void FireRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float /*deltaTime*/
    )
    {
        if (!IsGridValid()) return;

        ApplyDecay();
        InjectHeat(spectrum);
        PropagateFire();
    }

    void FireRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        if (!IsGridValid()) return;

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
        if (!CanInitializeGrid()) {
            m_gridWidth = 0;
            m_gridHeight = 0;
            m_fireGrid.clear();
            return;
        }

        m_gridWidth = CalculateGridWidth();
        m_gridHeight = CalculateGridHeight();

        const size_t gridSize = CalculateGridSize();

        if (gridSize > 0) {
            m_fireGrid.assign(gridSize, 0.0f);
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

    bool FireRenderer::CanInitializeGrid() const
    {
        return m_width > 0 && m_height > 0 && m_settings.pixelSize > 0.0f;
    }

    int FireRenderer::CalculateGridWidth() const
    {
        return static_cast<int>(m_width / m_settings.pixelSize);
    }

    int FireRenderer::CalculateGridHeight() const
    {
        return static_cast<int>(m_height / m_settings.pixelSize);
    }

    size_t FireRenderer::CalculateGridSize() const
    {
        if (m_gridWidth <= 0 || m_gridHeight <= 0) return 0;
        return static_cast<size_t>(m_gridWidth) * static_cast<size_t>(m_gridHeight);
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

        if (!IsBottomRowValid(bottomY)) return;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float normalized = NormalizedFloat(spectrum[i]);
            const int x = MapSpectrumIndexToGridX(i, spectrum.size());

            InjectHeatAtPosition(x, bottomY, normalized);
        }
    }

    void FireRenderer::InjectHeatAtPosition(
        int x,
        int bottomY,
        float normalizedValue
    )
    {
        const int clampedX = Helpers::Math::Clamp(x, 0, m_gridWidth - 1);
        const size_t idx = GetGridIndex(clampedX, bottomY);

        if (!IsGridIndexValid(idx)) return;

        m_fireGrid[idx] = std::max(
            m_fireGrid[idx],
            normalizedValue * m_settings.heatMultiplier
        );
    }

    void FireRenderer::PropagateFire()
    {
        const std::vector<float> readGrid = m_fireGrid;

        for (int y = 0; y < m_gridHeight - 1; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                PropagateCell(readGrid, x, y);
            }
        }
    }

    void FireRenderer::PropagateCell(
        const std::vector<float>& readGrid,
        int x,
        int y
    )
    {
        const int srcY = y + 1;
        int srcX = x;

        if (m_settings.useWind) {
            srcX += CalculateWindOffset(x, GetTime());
        }

        srcX = Helpers::Math::Clamp(srcX, 0, m_gridWidth - 1);

        float value = GetCellValue(readGrid, srcX, srcY);

        if (m_settings.useSmoothing) {
            value = CalculateSmoothedValue(readGrid, value, x, srcY);
        }

        const size_t destIdx = GetGridIndex(x, y);

        if (IsGridIndexValid(destIdx)) {
            m_fireGrid[destIdx] = value;
        }
    }

    float FireRenderer::GetCellValue(
        const std::vector<float>& grid,
        int x,
        int y
    ) const
    {
        const size_t idx = GetGridIndex(x, y);

        if (idx >= grid.size()) return 0.0f;

        return grid[idx];
    }

    int FireRenderer::CalculateWindOffset(
        int x,
        float time
    ) const
    {
        return static_cast<int>(
            std::sin(time * kWindSpeed + x * kWindPhaseOffset) * kWindAmplitude
            );
    }

    float FireRenderer::CalculateSmoothedValue(
        const std::vector<float>& readGrid,
        float centerValue,
        int x,
        int srcY
    ) const
    {
        const float leftValue = (x > 0)
            ? GetCellValue(readGrid, x - 1, srcY)
            : centerValue;

        const float rightValue = (x < m_gridWidth - 1)
            ? GetCellValue(readGrid, x + 1, srcY)
            : centerValue;

        return centerValue * kSmoothingCenter +
            (leftValue + rightValue) * kSmoothingSide;
    }

    int FireRenderer::MapSpectrumIndexToGridX(
        size_t spectrumIndex,
        size_t spectrumSize
    ) const
    {
        if (spectrumSize <= 1) return 0;

        const float mapped = Helpers::Math::Map(
            static_cast<float>(spectrumIndex),
            0.0f,
            static_cast<float>(spectrumSize - 1),
            0.0f,
            static_cast<float>(m_gridWidth - 1)
        );

        return static_cast<int>(mapped);
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
        const size_t idx = GetGridIndex(x, y);

        if (!IsGridIndexValid(idx)) return;

        const float intensity = m_fireGrid[idx];

        if (!IsPixelVisible(intensity)) return;

        const float clampedIntensity = Helpers::Math::Clamp(intensity, 0.0f, 1.0f);
        Color color = GetColorFromPalette(clampedIntensity);
        color = ApplyAlphaAdjustment(color, clampedIntensity);

        if (!IsColorVisible(color)) return;

        const Rect pixelRect = CalculatePixelRect(x, y);

        RenderPixelRect(canvas, pixelRect, color);
    }

    void FireRenderer::RenderPixelRect(
        Canvas& canvas,
        const Rect& pixelRect,
        const Color& color
    ) const
    {
        canvas.DrawRectangle(pixelRect, Paint::Fill(color));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Rect FireRenderer::CalculatePixelRect(
        int x,
        int y
    ) const
    {
        return {
            static_cast<float>(x) * m_settings.pixelSize,
            static_cast<float>(y) * m_settings.pixelSize,
            m_settings.pixelSize,
            m_settings.pixelSize
        };
    }

    size_t FireRenderer::GetGridIndex(
        int x,
        int y
    ) const
    {
        return static_cast<size_t>(y) * m_gridWidth + x;
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

        return InterpolatePaletteColors(i1, i2, t);
    }

    Color FireRenderer::ApplyAlphaAdjustment(
        Color color,
        float intensity
    ) const
    {
        const float alphaMultiplier = Helpers::Math::Map(
            Helpers::Math::Clamp(intensity, kAlphaSmoothStepMin, kAlphaSmoothStepMax),
            kAlphaSmoothStepMin,
            kAlphaSmoothStepMax,
            0.0f,
            1.0f
        );

        const float smoothed = alphaMultiplier * alphaMultiplier * (3.0f - 2.0f * alphaMultiplier);
        color.a *= smoothed;

        return color;
    }

    Color FireRenderer::InterpolatePaletteColors(
        size_t index1,
        size_t index2,
        float t
    ) const
    {
        return Helpers::Color::InterpolateColor(
            m_firePalette[index1],
            m_firePalette[index2],
            t
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool FireRenderer::IsGridValid() const
    {
        return m_gridWidth > 0 && m_gridHeight > 0;
    }

    bool FireRenderer::IsGridIndexValid(size_t index) const
    {
        return index < m_fireGrid.size();
    }

    bool FireRenderer::IsPixelVisible(float intensity) const
    {
        return intensity >= kMinVisibleIntensity;
    }

    bool FireRenderer::IsColorVisible(const Color& color) const
    {
        return color.a >= kMinVisibleAlpha;
    }

    bool FireRenderer::IsBottomRowValid(int bottomY) const
    {
        return bottomY >= 0;
    }

    bool FireRenderer::IsXInBounds(int x) const
    {
        return x >= 0 && x < m_gridWidth;
    }

} // namespace Spectrum