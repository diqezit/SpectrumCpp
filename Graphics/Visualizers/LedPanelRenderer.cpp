// LedPanelRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the LedPanelRenderer for LED matrix visualization.
//
// Implementation details:
// - Grid dimensions calculated from viewport and spectrum size
// - LED positions cached for batch rendering performance
// - Attack/decay smoothing creates realistic LED response
// - Peak indicators fade with timer (hold + decay)
// - Row colors pre-calculated from gradient for efficiency
//
// Rendering pipeline:
// 1. Render all inactive LEDs in single batch (background grid)
// 2. Render active LEDs (magnitude-driven)
// 3. Render peak indicators (quality-dependent, outline style)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "LedPanelRenderer.h"
#include "D2DHelpers.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "Canvas.h"

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kLedRadius = 6.0f;
        constexpr float kLedMargin = 3.0f;
        constexpr float kInactiveAlpha = 0.08f;
        constexpr float kMinActiveBrightness = 0.4f;
        constexpr float kDecayRate = 0.85f;
        constexpr float kAttackRate = 0.4f;
        constexpr float kPeakHoldTime = 0.5f;
        constexpr float kOverlayPaddingFactor = 0.95f;
        constexpr float kPeakStrokeWidth = 2.0f;
        constexpr float kPeakRadiusOffset = 2.0f;
        constexpr float kPeakDecayRate = 0.95f;
        constexpr float kMinValueThreshold = 0.05f;
        constexpr float kTopLedBrightnessBoost = 1.2f;
        constexpr float kExternalColorBlend = 0.7f;

        constexpr int kMinGridSize = 10;
        constexpr int kMaxColumns = 64;

        const std::vector<Color> kSpectrumGradient = {
            Color::FromRGB(0, 200, 100),
            Color::FromRGB(0, 255, 0),
            Color::FromRGB(128, 255, 0),
            Color::FromRGB(255, 255, 0),
            Color::FromRGB(255, 200, 0),
            Color::FromRGB(255, 128, 0),
            Color::FromRGB(255, 64, 0),
            Color::FromRGB(255, 0, 0),
            Color::FromRGB(200, 0, 50)
        };

        const Color kInactiveColor = Color::FromRGB(80, 80, 80);
        const Color kPeakColor = Color(1.0f, 1.0f, 1.0f, 200.0f / 255.0f);

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LedPanelRenderer::LedPanelRenderer()
    {
        UpdateSettings();
    }

    void LedPanelRenderer::OnActivate(int width, int height)
    {
        BaseRenderer::OnActivate(width, height);
        m_grid.columns = 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::UpdateSettings()
    {
        if (m_isOverlay) {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = { true, 8, 1.2f };
                break;
            case RenderQuality::High:
                m_settings = { true, 16, 1.0f };
                break;
            case RenderQuality::Medium:
            default:
                m_settings = { true, 12, 1.1f };
                break;
            }
        }
        else {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = { false, 16, 1.0f };
                break;
            case RenderQuality::High:
                m_settings = { true, 32, 0.8f };
                break;
            case RenderQuality::Medium:
            default:
                m_settings = { true, 24, 0.9f };
                break;
            }
        }

        m_grid.columns = 0;
    }

    void LedPanelRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        UpdateGrid(spectrum.size());

        if (m_grid.columns == 0) return;

        UpdateValues(spectrum);

        if (m_settings.usePeakHold) {
            UpdatePeakValues(deltaTime);
        }
    }

    void LedPanelRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        if (m_grid.columns == 0) return;

        RenderInactiveLeds(canvas);
        RenderActiveLeds(canvas);

        if (m_settings.usePeakHold) {
            RenderPeakLeds(canvas);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Grid Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::UpdateGrid(size_t requiredColumns)
    {
        if (m_grid.columns > 0 &&
            static_cast<size_t>(m_grid.columns) >= requiredColumns) {
            return;
        }

        if (m_width <= 0 || m_height <= 0 || requiredColumns == 0) return;

        const float ledSize = kLedRadius * 2.0f + kLedMargin;
        const float availableWidth = m_isOverlay
            ? m_width * kOverlayPaddingFactor
            : static_cast<float>(m_width);
        const float availableHeight = m_isOverlay
            ? m_height * kOverlayPaddingFactor
            : static_cast<float>(m_height);

        int cols = std::min({
            kMaxColumns,
            static_cast<int>(requiredColumns),
            static_cast<int>(availableWidth / ledSize)
            });
        cols = std::max(kMinGridSize, cols);

        int rows = std::min(
            m_settings.maxRows,
            static_cast<int>(availableHeight / ledSize)
        );
        rows = std::max(kMinGridSize, rows);

        const float cellSize = std::min(
            availableWidth / cols,
            availableHeight / rows
        );
        const float gridWidth = cols * cellSize;
        const float gridHeight = rows * cellSize;

        GridData newGrid;
        newGrid.rows = rows;
        newGrid.columns = cols;
        newGrid.cellSize = cellSize;
        newGrid.startX = (m_width - gridWidth) * 0.5f;
        newGrid.startY = (m_height - gridHeight) * 0.5f;

        CreateGrid(newGrid);
    }

    void LedPanelRenderer::CreateGrid(const GridData& gridData)
    {
        m_grid = gridData;

        m_smoothedValues.assign(m_grid.columns, 0.0f);
        m_peakValues.assign(m_grid.columns, 0.0f);
        m_peakTimers.assign(m_grid.columns, 0.0f);

        CacheLedPositions();
        InitializeRowColors();
    }

    void LedPanelRenderer::CacheLedPositions()
    {
        m_allLedPositions.clear();
        m_allLedPositions.reserve(
            static_cast<size_t>(m_grid.columns) * m_grid.rows
        );

        const float halfCell = m_grid.cellSize * 0.5f;

        for (int col = 0; col < m_grid.columns; ++col) {
            const float x = m_grid.startX + col * m_grid.cellSize + halfCell;

            for (int row = 0; row < m_grid.rows; ++row) {
                const float y = m_grid.startY
                    + (m_grid.rows - 1 - row) * m_grid.cellSize
                    + halfCell;

                m_allLedPositions.push_back({ x, y });
            }
        }
    }

    void LedPanelRenderer::InitializeRowColors()
    {
        m_rowColors.resize(m_grid.rows);

        for (int i = 0; i < m_grid.rows; ++i) {
            const float t = (m_grid.rows > 1)
                ? static_cast<float>(i) / (m_grid.rows - 1)
                : 0.0f;

            m_rowColors[i] = InterpolateGradient(t);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::UpdateValues(const SpectrumData& spectrum)
    {
        const size_t count = std::min(
            static_cast<size_t>(m_grid.columns),
            spectrum.size()
        );

        for (size_t i = 0; i < count; ++i) {
            const float current = m_smoothedValues[i];
            const float target = spectrum[i];
            const float baseRate = (current < target) ? kAttackRate : (1.0f - kDecayRate);
            const float rate = baseRate * m_settings.smoothingMultiplier;

            m_smoothedValues[i] = Utils::Lerp(current, target, rate);
        }
    }

    void LedPanelRenderer::UpdatePeakValues(float deltaTime)
    {
        for (int i = 0; i < m_grid.columns; ++i) {
            if (m_smoothedValues[i] >= m_peakValues[i]) {
                m_peakValues[i] = m_smoothedValues[i];
                m_peakTimers[i] = kPeakHoldTime;
            }
            else if (m_peakTimers[i] > 0.0f) {
                m_peakTimers[i] -= deltaTime;
            }
            else {
                m_peakValues[i] *= kPeakDecayRate;
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::RenderInactiveLeds(Canvas& canvas) const
    {
        Color color = kInactiveColor;
        color.a = kInactiveAlpha;

        if (m_isOverlay) {
            color.a *= kOverlayPaddingFactor;
        }

        const Paint paint{ color, true };
        canvas.DrawCircleBatch(m_allLedPositions, kLedRadius, paint);
    }

    void LedPanelRenderer::RenderActiveLeds(Canvas& canvas) const
    {
        for (int col = 0; col < m_grid.columns; ++col) {
            const float value = m_smoothedValues[col];
            int activeLeds = static_cast<int>(value * m_grid.rows);

            if (activeLeds == 0 && value > kMinValueThreshold) {
                activeLeds = 1;
            }

            const float brightness = Utils::Lerp(kMinActiveBrightness, 1.0f, value);

            for (int row = 0; row < activeLeds; ++row) {
                const bool isTopLed = (row == activeLeds - 1);
                const float ledBrightness = isTopLed
                    ? brightness * kTopLedBrightnessBoost
                    : brightness;

                const Color ledColor = GetLedColor(row, Utils::Saturate(ledBrightness));
                const size_t ledIndex = static_cast<size_t>(col) * m_grid.rows + row;

                const Paint paint{ ledColor, true };
                canvas.DrawCircle(m_allLedPositions[ledIndex], kLedRadius, paint);
            }
        }
    }

    void LedPanelRenderer::RenderPeakLeds(Canvas& canvas) const
    {
        for (int col = 0; col < m_grid.columns; ++col) {
            if (m_peakTimers[col] <= 0.0f) continue;

            const int peakRow = static_cast<int>(m_peakValues[col] * m_grid.rows) - 1;

            if (peakRow >= 0 && peakRow < m_grid.rows) {
                const size_t ledIndex = static_cast<size_t>(col) 
                    * static_cast<size_t>(m_grid.rows) 
                    + static_cast<size_t>(peakRow);

                const Paint paint{ kPeakColor, kPeakStrokeWidth, false };
                canvas.DrawCircle(
                    m_allLedPositions[ledIndex],
                    kLedRadius + kPeakRadiusOffset,
                    paint
                );
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color LedPanelRenderer::GetLedColor(int row, float brightness) const
    {
        const int rowIndex = std::min(row, static_cast<int>(m_rowColors.size()) - 1);

        if (rowIndex < 0) return {};

        Color baseColor = m_rowColors[rowIndex];

        if (HasExternalColor()) {
            const float t = (m_rowColors.size() > 1)
                ? static_cast<float>(row) / (m_rowColors.size() - 1)
                : 0.0f;

            baseColor = BlendWithExternalColor(baseColor, t);
        }

        baseColor.a = brightness;
        return baseColor;
    }

    bool LedPanelRenderer::HasExternalColor() const
    {
        return (m_primaryColor.r != 1.0f ||
            m_primaryColor.g != 1.0f ||
            m_primaryColor.b != 1.0f);
    }

    Color LedPanelRenderer::BlendWithExternalColor(Color baseColor, float t) const
    {
        const auto blend = [](float ext, float grad, float t_) {
            return ext * kExternalColorBlend +
                grad * (1.0f - kExternalColorBlend) * t_;
            };

        return Color(
            blend(m_primaryColor.r, baseColor.r, t),
            blend(m_primaryColor.g, baseColor.g, t),
            blend(m_primaryColor.b, baseColor.b, t)
        );
    }

    Color LedPanelRenderer::InterpolateGradient(float t)
    {
        const float scaledT = t * (kSpectrumGradient.size() - 1);
        const int index = static_cast<int>(scaledT);
        const float fraction = scaledT - index;

        if (index >= static_cast<int>(kSpectrumGradient.size()) - 1) {
            return kSpectrumGradient.back();
        }

        return Utils::InterpolateColor(
            kSpectrumGradient[index],
            kSpectrumGradient[index + 1],
            fraction
        );
    }

} // namespace Spectrum