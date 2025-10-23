// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the MatrixLedRenderer for grid-based LED matrix visualization.
//
// Implementation details:
// - Grid dimensions calculated from viewport and spectrum size
// - LED positions cached in m_allLedRects for batch rendering
// - Attack/decay smoothing creates realistic LED response behavior
// - Peak tracking managed by PeakTracker component (DRY principle)
// - Row colors pre-calculated from gradient for rendering efficiency
// - Batch rendering minimizes draw calls by grouping similar colors
// - Uses GeometryHelpers for all geometric operations
//
// Rendering pipeline:
// 1. Background: render all inactive LEDs in single batch
// 2. Active LEDs: group by color and render in batches
// 3. Peak indicators: collect visible peaks and render in batch
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/MatrixLedRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

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

        // Fixed colors
        const Color kInactiveColor = Color::FromRGB(80, 80, 80);
        const Color kPeakColor = Color(1.0f, 1.0f, 1.0f, 200.0f / 255.0f);

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    MatrixLedRenderer::MatrixLedRenderer()
        : m_peakTracker(0, CreatePeakConfig(kPeakHoldTime, kPeakDecayRate, 0.01f))
    {
        UpdateSettings();
    }

    void MatrixLedRenderer::OnActivate(
        int width,
        int height
    )
    {
        BaseRenderer::OnActivate(width, height);
        m_grid.columns = 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MatrixLedRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<MatrixLedRenderer>(m_quality, m_isOverlay);
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

        if (m_settings.enableGlow) {
            m_peakTracker.Update(m_smoothedValues, deltaTime);
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

        if (m_settings.enableGlow) {
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

        m_smoothedValues.assign(m_grid.columns, 0.0f);
        m_peakTracker.Resize(m_grid.columns);

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
                const Point cellOffset = {
                    col * m_grid.cellSize + margin,
                    (m_grid.rows - 1 - row) * m_grid.cellSize + margin
                };

                const Point ledPos = Add(m_grid.gridStart, cellOffset);

                m_allLedRects.push_back({ ledPos.x, ledPos.y, ledSize, ledSize });
            }
        }
    }

    void MatrixLedRenderer::PrecomputeRowColors()
    {
        m_rowColors.resize(m_grid.rows);

        for (int i = 0; i < m_grid.rows; ++i) {
            const float t = (m_grid.rows > 1)
                ? Helpers::Math::Normalize(static_cast<float>(i), 0.0f, static_cast<float>(m_grid.rows - 1))
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

        data.columns = Helpers::Math::Clamp(
            std::min(
                static_cast<int>(requiredColumns),
                static_cast<int>(availableWidth / ledSize)
            ),
            kMinGridSize,
            maxAllowedCols
        );

        data.rows = Helpers::Math::Clamp(
            static_cast<int>(availableHeight / ledSize),
            kMinGridSize,
            m_settings.ledDensity
        );

        data.cellSize = std::min(
            availableWidth / data.columns,
            availableHeight / data.rows
        );

        const float gridWidth = data.columns * data.cellSize;
        const float gridHeight = data.rows * data.cellSize;

        const Point viewportCenter = GetViewportCenter(m_width, m_height);
        const Point gridSize = { gridWidth, gridHeight };
        const Point halfGridSize = Multiply(gridSize, 0.5f);

        data.gridStart = Subtract(viewportCenter, halfGridSize);
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

    Point MatrixLedRenderer::CalculateLedCenter(int column, int row) const
    {
        const float cellHalf = m_grid.cellSize * 0.5f;

        const Point cellOffset = {
            column * m_grid.cellSize + cellHalf,
            (m_grid.rows - 1 - row) * m_grid.cellSize + cellHalf
        };

        return Add(m_grid.gridStart, cellOffset);
    }

    Rect MatrixLedRenderer::GetGridBounds() const
    {
        const float gridWidth = m_grid.columns * m_grid.cellSize;
        const float gridHeight = m_grid.rows * m_grid.cellSize;

        return {
            m_grid.gridStart.x,
            m_grid.gridStart.y,
            gridWidth,
            gridHeight
        };
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

    void MatrixLedRenderer::UpdateSingleValue(size_t index, float target)
    {
        const float current = m_smoothedValues[index];

        const float attackRate = kAttackRate * m_settings.blurAmount;
        const float decayRate = 1.0f - (kDecayRate * m_settings.blurAmount);

        const float rate = (current < target) ? attackRate : decayRate;

        m_smoothedValues[index] = Helpers::Math::Lerp(current, target, rate);
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
                    Saturate(ledBrightness)
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
            if (!m_peakTracker.IsPeakVisible(col)) continue;

            const int peakRow = GetPeakLedRow(m_peakTracker.GetPeak(col));

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
        return GetTopLeft(rect);
    }

    int MatrixLedRenderer::GetActiveLedCount(float value) const
    {
        int count = static_cast<int>(value * m_grid.rows);

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
                ? Helpers::Math::Normalize(static_cast<float>(row), 0.0f, static_cast<float>(m_rowColors.size() - 1))
                : 0.0f;

            baseColor = ApplyExternalColorBlend(baseColor, t);
        }

        baseColor.a = brightness;
        return baseColor;
    }

    Color MatrixLedRenderer::GetRowBaseColor(int row) const
    {
        const int rowIndex = Helpers::Math::Clamp(
            row,
            0,
            static_cast<int>(m_rowColors.size()) - 1
        );

        return m_rowColors[rowIndex];
    }

    Color MatrixLedRenderer::ApplyExternalColorBlend(
        Color baseColor,
        float t
    ) const
    {
        auto blend = [&](float ext, float grad, float intensity) {
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

        return Helpers::Color::InterpolateColor(
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
        return Helpers::Math::Lerp(kMinActiveBrightness, 1.0f, value);
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

} // namespace Spectrum