// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the MatrixLedRenderer for grid-based LED matrix visualization.
//
// Implementation details:
// - Grid dimensions calculated from viewport and spectrum size
// - LED positions cached in m_allLedRects for batch rendering
// - Attack/decay smoothing creates realistic LED response behavior
// - Peak tracking uses hold timer followed by exponential decay
// - Row colors pre-calculated from gradient for rendering efficiency
// - Batch rendering minimizes draw calls by grouping similar colors
//
// Rendering pipeline:
// 1. Background: render all inactive LEDs in single batch
// 2. Active LEDs: group by color and render in batches
// 3. Peak indicators: collect visible peaks and render in batch
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/MatrixLedRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        // LED appearance
        constexpr float kLedRadius = 2.0f;
        constexpr float kLedMargin = 1.0f;
        constexpr float kInactiveAlpha = 0.08f;
        constexpr float kMinActiveBrightness = 0.4f;
        constexpr float kTopLedBrightnessBoost = 1.2f;

        // Animation parameters
        constexpr float kDecayRate = 0.85f;
        constexpr float kAttackRate = 0.4f;
        constexpr float kPeakHoldTime = 0.5f;
        constexpr float kPeakDecayRate = 0.95f;
        constexpr float kMinValueThreshold = 0.05f;

        // Color blending
        constexpr float kExternalColorBlend = 0.7f;
        constexpr float kOverlayPaddingFactor = 0.95f;

        // Grid constraints
        constexpr int kMinGridSize = 10;
        constexpr int kMaxColumns = 64;

        // Spectrum gradient (green to red)
        const std::vector<Color> kSpectrumGradient = {
            {0, 200, 100},   // Dark green
            {0, 255, 0},     // Green
            {128, 255, 0},   // Yellow-green
            {255, 255, 0},   // Yellow
            {255, 200, 0},   // Orange-yellow
            {255, 128, 0},   // Orange
            {255, 64, 0},    // Red-orange
            {255, 0, 0},     // Red
            {200, 0, 50}     // Dark red
        };

        // Fixed colors
        const Color kInactiveColor = { 80, 80, 80 };
        const Color kPeakColor = { 255, 255, 255, 200 };

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    MatrixLedRenderer::MatrixLedRenderer()
    {
        UpdateSettings();
    }

    void MatrixLedRenderer::OnActivate(
        int width,
        int height
    )
    {
        BaseRenderer::OnActivate(width, height);

        // Force grid recreation
        m_grid.columns = 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MatrixLedRenderer::UpdateSettings()
    {
        if (m_isOverlay) {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = {
                    false,  // usePeakHold
                    16,     // maxRows
                    1.0f    // smoothingMultiplier
                };
                break;
            case RenderQuality::High:
                m_settings = {
                    true,   // usePeakHold
                    32,     // maxRows
                    0.8f    // smoothingMultiplier
                };
                break;
            case RenderQuality::Medium:
            default:
                m_settings = {
                    true,   // usePeakHold
                    24,     // maxRows
                    0.9f    // smoothingMultiplier
                };
                break;
            }
        }
        else {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = {
                    false,  // usePeakHold
                    16,     // maxRows
                    1.0f    // smoothingMultiplier
                };
                break;
            case RenderQuality::High:
                m_settings = {
                    true,   // usePeakHold
                    32,     // maxRows
                    0.8f    // smoothingMultiplier
                };
                break;
            case RenderQuality::Medium:
            default:
                m_settings = {
                    true,   // usePeakHold
                    24,     // maxRows
                    0.9f    // smoothingMultiplier
                };
                break;
            }
        }

        // Force grid recreation
        m_grid.columns = 0;
    }

    void MatrixLedRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        UpdateGridIfNeeded(spectrum.size());

        if (!IsGridValid()) return;

        UpdateSmoothedValues(spectrum);

        if (m_settings.usePeakHold) {
            UpdatePeakTracking(deltaTime);
        }
    }

    void MatrixLedRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        if (!IsGridValid()) return;

        RenderInactiveLeds(canvas);
        RenderActiveLeds(canvas);

        if (m_settings.usePeakHold) {
            RenderPeakIndicators(canvas);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Grid Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MatrixLedRenderer::UpdateGridIfNeeded(size_t requiredColumns)
    {
        if (!RequiresGridUpdate(requiredColumns)) return;

        const GridData newGrid = CalculateGridLayout(requiredColumns);
        RecreateGrid(newGrid);
    }

    void MatrixLedRenderer::RecreateGrid(const GridData& gridData)
    {
        m_grid = gridData;

        // Resize state arrays
        m_smoothedValues.assign(m_grid.columns, 0.0f);
        m_peakValues.assign(m_grid.columns, 0.0f);
        m_peakTimers.assign(m_grid.columns, 0.0f);

        // Cache geometry and colors
        CacheLedPositions();
        PrecomputeRowColors();
    }

    void MatrixLedRenderer::CacheLedPositions()
    {
        m_allLedRects.clear();
        m_allLedRects.reserve(
            static_cast<size_t>(m_grid.columns) * static_cast<size_t>(m_grid.rows)
        );

        const float ledSize = m_grid.cellSize - kLedMargin;
        const float margin = kLedMargin * 0.5f;

        for (int col = 0; col < m_grid.columns; ++col) {
            for (int row = 0; row < m_grid.rows; ++row) {
                const float x = m_grid.startX + col * m_grid.cellSize + margin;
                const float y = m_grid.startY + (m_grid.rows - 1 - row) * m_grid.cellSize + margin;

                m_allLedRects.push_back({ x, y, ledSize, ledSize });
            }
        }
    }

    void MatrixLedRenderer::PrecomputeRowColors()
    {
        m_rowColors.resize(m_grid.rows);

        for (int i = 0; i < m_grid.rows; ++i) {
            const float t = (m_grid.rows > 1)
                ? static_cast<float>(i) / (m_grid.rows - 1)
                : 0.0f;

            m_rowColors[i] = InterpolateGradient(t);
        }
    }

    MatrixLedRenderer::GridData MatrixLedRenderer::CalculateGridLayout(
        size_t requiredColumns
    ) const
    {
        GridData data;

        const float ledSize = kLedRadius * 2.0f + kLedMargin;

        const float availableWidth = m_isOverlay
            ? m_width * kOverlayPaddingFactor
            : static_cast<float>(m_width);

        const float availableHeight = m_isOverlay
            ? m_height * kOverlayPaddingFactor
            : static_cast<float>(m_height);

        const int maxAllowedCols = std::min(
            static_cast<int>(kMaxColumns),
            RenderUtils::GetMaxBarsForQuality(m_quality)
        );

        // Calculate column count
        data.columns = std::min({
            maxAllowedCols,
            static_cast<int>(requiredColumns),
            static_cast<int>(availableWidth / ledSize)
            });
        data.columns = std::max(kMinGridSize, data.columns);

        // Calculate row count
        data.rows = std::min(
            m_settings.maxRows,
            static_cast<int>(availableHeight / ledSize)
        );
        data.rows = std::max(kMinGridSize, data.rows);

        // Calculate cell size
        data.cellSize = std::min(
            availableWidth / data.columns,
            availableHeight / data.rows
        );

        // Calculate grid position (centered)
        const float gridWidth = data.columns * data.cellSize;
        const float gridHeight = data.rows * data.cellSize;

        data.startX = (m_width - gridWidth) * 0.5f;
        data.startY = (m_height - gridHeight) * 0.5f;

        data.isOverlay = m_isOverlay;

        return data;
    }

    bool MatrixLedRenderer::RequiresGridUpdate(size_t requiredColumns) const
    {
        if (m_grid.columns == 0) return true;

        const GridData newGrid = CalculateGridLayout(requiredColumns);

        return m_grid.columns != newGrid.columns ||
            m_grid.rows != newGrid.rows ||
            std::abs(m_grid.cellSize - newGrid.cellSize) > 0.1f ||
            m_grid.isOverlay != newGrid.isOverlay;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MatrixLedRenderer::UpdateSmoothedValues(const SpectrumData& spectrum)
    {
        const size_t count = std::min(
            static_cast<size_t>(m_grid.columns),
            spectrum.size()
        );

        for (size_t i = 0; i < count; ++i) {
            UpdateSingleValue(i, spectrum[i]);
        }
    }

    void MatrixLedRenderer::UpdatePeakTracking(float deltaTime)
    {
        for (int i = 0; i < m_grid.columns; ++i) {
            UpdateSinglePeak(i, deltaTime);
        }
    }

    void MatrixLedRenderer::UpdateSingleValue(size_t index, float target)
    {
        const float current = m_smoothedValues[index];

        const float attackRate = kAttackRate * m_settings.smoothingMultiplier;
        const float decayRate = 1.0f - (kDecayRate * m_settings.smoothingMultiplier);

        const float rate = (current < target) ? attackRate : decayRate;

        m_smoothedValues[index] = Utils::Lerp(current, target, rate);
    }

    void MatrixLedRenderer::UpdateSinglePeak(size_t index, float deltaTime)
    {
        const float currentValue = m_smoothedValues[index];

        if (currentValue > m_peakValues[index]) {
            // New peak detected
            m_peakValues[index] = currentValue;
            m_peakTimers[index] = kPeakHoldTime;
        }
        else if (m_peakTimers[index] > 0.0f) {
            // Hold phase
            m_peakTimers[index] -= deltaTime;
        }
        else {
            // Decay phase
            m_peakValues[index] *= kPeakDecayRate;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Rendering Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MatrixLedRenderer::RenderInactiveLeds(Canvas& canvas) const
    {
        const Color color = GetInactiveLedColor();

        canvas.DrawRectangleBatch(
            m_allLedRects,
            Paint::Fill(color)
        );
    }

    void MatrixLedRenderer::RenderActiveLeds(Canvas& canvas) const
    {
        const auto batches = GroupActiveLedsByColor();

        RenderLedBatches(canvas, batches);
    }

    void MatrixLedRenderer::RenderPeakIndicators(Canvas& canvas) const
    {
        const auto peakRects = CollectPeakLeds();

        if (!peakRects.empty()) {
            RenderPeakBatch(canvas, peakRects);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Batch Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MatrixLedRenderer::RenderLedBatches(
        Canvas& canvas,
        const std::map<Color, std::vector<Rect>>& batches
    ) const
    {
        for (const auto& [color, rects] : batches) {
            canvas.DrawRectangleBatch(rects, Paint::Fill(color));
        }
    }

    void MatrixLedRenderer::RenderPeakBatch(
        Canvas& canvas,
        const std::vector<Rect>& peakRects
    ) const
    {
        const Color color = GetPeakLedColor();

        canvas.DrawRectangleBatch(peakRects, Paint::Fill(color));
    }

    std::map<Color, std::vector<Rect>>
        MatrixLedRenderer::GroupActiveLedsByColor() const
    {
        std::map<Color, std::vector<Rect>> batches;

        for (int col = 0; col < m_grid.columns; ++col) {
            const float value = m_smoothedValues[col];
            const int activeLeds = GetActiveLedCount(value);
            const float brightness = CalculateLedBrightness(value);

            for (int row = 0; row < activeLeds; ++row) {
                const bool isTopLed = (row == activeLeds - 1);

                const float ledBrightness = isTopLed
                    ? CalculateTopLedBrightness(brightness)
                    : brightness;

                const Color ledColor = CalculateLedColor(
                    row,
                    Utils::Saturate(ledBrightness)
                );

                batches[ledColor].push_back(GetLedRect(col, row));
            }
        }

        return batches;
    }

    std::vector<Rect> MatrixLedRenderer::CollectPeakLeds() const
    {
        std::vector<Rect> peakRects;

        for (int col = 0; col < m_grid.columns; ++col) {
            if (!IsPeakVisible(col)) continue;

            const int peakRow = GetPeakLedRow(m_peakValues[col]);

            if (IsRowValid(peakRow)) {
                peakRects.push_back(GetLedRect(col, peakRow));
            }
        }

        return peakRects;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // LED Position Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Rect MatrixLedRenderer::GetLedRect(int column, int row) const
    {
        const size_t colIndex = static_cast<size_t>(column);
        const size_t rowIndex = static_cast<size_t>(row);
        const size_t gridRows = static_cast<size_t>(m_grid.rows);
        const size_t index = colIndex * gridRows + rowIndex;

        return m_allLedRects[index];
    }

    Point MatrixLedRenderer::GetLedPosition(int column, int row) const
    {
        const Rect rect = GetLedRect(column, row);
        return { rect.x, rect.y };
    }

    int MatrixLedRenderer::GetActiveLedCount(float value) const
    {
        int count = static_cast<int>(value * m_grid.rows);

        // Show at least one LED if value is above threshold
        if (count == 0 && value > kMinValueThreshold) {
            count = 1;
        }

        return count;
    }

    int MatrixLedRenderer::GetPeakLedRow(float peakValue) const
    {
        return static_cast<int>(peakValue * m_grid.rows) - 1;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color MatrixLedRenderer::CalculateLedColor(int row, float brightness) const
    {
        Color baseColor = GetRowBaseColor(row);

        if (HasExternalColor()) {
            const float t = (m_rowColors.size() > 1)
                ? static_cast<float>(row) / (m_rowColors.size() - 1)
                : 0.0f;

            baseColor = ApplyExternalColorBlend(baseColor, t);
        }

        baseColor.a = brightness;
        return baseColor;
    }

    Color MatrixLedRenderer::GetRowBaseColor(int row) const
    {
        const int rowIndex = std::min(
            row,
            static_cast<int>(m_rowColors.size()) - 1
        );

        if (rowIndex < 0) return {};

        return m_rowColors[rowIndex];
    }

    Color MatrixLedRenderer::ApplyExternalColorBlend(
        Color baseColor,
        float t
    ) const
    {
        auto blend = [](float ext, float grad, float intensity) {
            return ext * kExternalColorBlend +
                grad * (1.0f - kExternalColorBlend) * intensity;
            };

        return {
            blend(m_primaryColor.r, baseColor.r, t),
            blend(m_primaryColor.g, baseColor.g, t),
            blend(m_primaryColor.b, baseColor.b, t)
        };
    }

    Color MatrixLedRenderer::GetInactiveLedColor() const
    {
        Color color = kInactiveColor;
        color.a = kInactiveAlpha;

        if (m_isOverlay) {
            color.a *= kOverlayPaddingFactor;
        }

        return color;
    }

    Color MatrixLedRenderer::GetPeakLedColor() const
    {
        Color color = kPeakColor;

        if (m_isOverlay) {
            color.a *= kOverlayPaddingFactor;
        }

        return color;
    }

    bool MatrixLedRenderer::HasExternalColor() const
    {
        return m_primaryColor.r != 1.0f ||
            m_primaryColor.g != 1.0f ||
            m_primaryColor.b != 1.0f;
    }

    Color MatrixLedRenderer::InterpolateGradient(float t)
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Brightness Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float MatrixLedRenderer::CalculateLedBrightness(float value) const
    {
        return Utils::Lerp(kMinActiveBrightness, 1.0f, value);
    }

    float MatrixLedRenderer::CalculateTopLedBrightness(float brightness) const
    {
        return brightness * kTopLedBrightnessBoost;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool MatrixLedRenderer::IsGridValid() const
    {
        return m_grid.columns > 0 && m_grid.rows > 0;
    }

    bool MatrixLedRenderer::IsColumnValid(int column) const
    {
        return column >= 0 && column < m_grid.columns;
    }

    bool MatrixLedRenderer::IsRowValid(int row) const
    {
        return row >= 0 && row < m_grid.rows;
    }

    bool MatrixLedRenderer::IsPeakVisible(int column) const
    {
        return IsColumnValid(column) && m_peakTimers[column] > 0.0f;
    }

} // namespace Spectrum