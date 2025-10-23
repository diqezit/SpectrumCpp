// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the LedPanelRenderer for LED matrix visualization.
//
// Implementation details:
// - Grid dimensions calculated from viewport and spectrum size
// - LED positions cached for batch rendering performance
// - Attack/decay smoothing creates realistic LED response
// - Peak indicators managed by PeakTracker component (DRY principle)
// - Row colors pre-calculated from gradient for efficiency
// - Uses GeometryHelpers for all geometric operations
//
// Rendering pipeline:
// 1. Render all inactive LEDs in single batch (background grid)
// 2. Render active LEDs (magnitude-driven)
// 3. Render peak indicators (quality-dependent, outline style)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <algorithm>

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kLedRadius = 6.0f;
        constexpr float kLedMargin = 3.0f;
        constexpr float kLedDiameter = kLedRadius * 2.0f;

        constexpr float kInactiveAlpha = 0.08f;
        constexpr float kMinActiveBrightness = 0.4f;
        constexpr float kTopLedBrightnessBoost = 1.2f;

        constexpr float kDecayRate = 0.85f;
        constexpr float kAttackRate = 0.4f;

        constexpr float kPeakHoldTime = 0.5f;
        constexpr float kPeakDecayRate = 0.95f;
        constexpr float kPeakStrokeWidth = 2.0f;
        constexpr float kPeakRadiusOffset = 2.0f;

        constexpr float kOverlayPaddingFactor = 0.95f;
        constexpr float kMinValueThreshold = 0.05f;
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

        const Color kInactiveColorBase = Color::FromRGB(80, 80, 80);
        const Color kPeakColorBase = Color(1.0f, 1.0f, 1.0f, 200.0f / 255.0f);

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LedPanelRenderer::LedPanelRenderer()
        : m_peakTracker(0, CreatePeakConfig(kPeakHoldTime, kPeakDecayRate, 0.01f))
    {
        UpdateSettings();
    }

    void LedPanelRenderer::OnActivate(
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

    void LedPanelRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<LedPanelRenderer>(m_quality, m_isOverlay);
        m_grid.columns = 0;
    }

    void LedPanelRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    )
    {
        UpdateGrid(spectrum.size());

        if (!IsGridValid()) return;

        UpdateValues(spectrum);

        if (m_settings.usePeakHold) {
            m_peakTracker.Update(m_smoothedValues, deltaTime);
        }
    }

    void LedPanelRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        if (!IsGridValid()) return;

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
        if (!ShouldRecreateGrid(requiredColumns)) return;

        if (!CanUpdateGrid(requiredColumns)) return;

        const GridData newGrid = CalculateGridData(requiredColumns);

        CreateGrid(newGrid);
    }

    void LedPanelRenderer::CreateGrid(const GridData& gridData)
    {
        m_grid = gridData;

        m_smoothedValues.assign(m_grid.columns, 0.0f);
        m_peakTracker.Resize(m_grid.columns);

        CacheLedPositions();
        InitializeRowColors();
    }

    void LedPanelRenderer::CacheLedPositions()
    {
        const size_t totalLeds = CalculateTotalLedCount();

        m_allLedPositions.clear();
        m_allLedPositions.reserve(totalLeds);

        for (int col = 0; col < m_grid.columns; ++col) {
            for (int row = 0; row < m_grid.rows; ++row) {
                m_allLedPositions.push_back(CalculateLedPosition(col, row));
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

    LedPanelRenderer::GridData LedPanelRenderer::CalculateGridData(size_t requiredColumns) const
    {
        const float ledSize = GetLedSize();
        const float availableWidth = GetAvailableWidth();
        const float availableHeight = GetAvailableHeight();

        const int cols = CalculateGridColumns(requiredColumns, availableWidth, ledSize);
        const int rows = CalculateGridRows(availableHeight, ledSize);

        const float cellSize = CalculateCellSize(cols, rows, availableWidth, availableHeight);
        const float gridWidth = cols * cellSize;
        const float gridHeight = rows * cellSize;

        const Point viewportCenter = GetViewportCenter(m_width, m_height);

        GridData grid;
        grid.rows = rows;
        grid.columns = cols;
        grid.cellSize = cellSize;
        grid.gridStart = {
            viewportCenter.x - gridWidth * 0.5f,
            viewportCenter.y - gridHeight * 0.5f
        };

        return grid;
    }

    int LedPanelRenderer::CalculateGridColumns(
        size_t requiredColumns,
        float availableWidth,
        float ledSize
    ) const
    {
        int cols = std::min({
            kMaxColumns,
            static_cast<int>(requiredColumns),
            static_cast<int>(availableWidth / ledSize)
            });

        return std::max(kMinGridSize, cols);
    }

    int LedPanelRenderer::CalculateGridRows(
        float availableHeight,
        float ledSize
    ) const
    {
        int rows = std::min(
            m_settings.maxRows,
            static_cast<int>(availableHeight / ledSize)
        );

        return std::max(kMinGridSize, rows);
    }

    float LedPanelRenderer::CalculateCellSize(
        int cols,
        int rows,
        float availableWidth,
        float availableHeight
    ) const
    {
        return std::min(
            availableWidth / cols,
            availableHeight / rows
        );
    }

    float LedPanelRenderer::GetAvailableWidth() const
    {
        return m_isOverlay
            ? m_width * kOverlayPaddingFactor
            : static_cast<float>(m_width);
    }

    float LedPanelRenderer::GetAvailableHeight() const
    {
        return m_isOverlay
            ? m_height * kOverlayPaddingFactor
            : static_cast<float>(m_height);
    }

    float LedPanelRenderer::GetLedSize() const
    {
        return kLedDiameter + kLedMargin;
    }

    Point LedPanelRenderer::CalculateLedPosition(
        int col,
        int row
    ) const
    {
        const float halfCell = m_grid.cellSize * 0.5f;

        const Point cellOffset = {
            col * m_grid.cellSize + halfCell,
            (m_grid.rows - 1 - row) * m_grid.cellSize + halfCell
        };

        return Add(m_grid.gridStart, cellOffset);
    }

    size_t LedPanelRenderer::CalculateTotalLedCount() const
    {
        return static_cast<size_t>(m_grid.columns) * m_grid.rows;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::UpdateValues(const SpectrumData& spectrum)
    {
        const size_t count = GetUpdateCount(spectrum);

        for (size_t i = 0; i < count; ++i) {
            UpdateColumnValue(i, spectrum[i]);
        }
    }

    void LedPanelRenderer::UpdateColumnValue(
        size_t index,
        float targetValue
    )
    {
        if (!IsColumnIndexValid(index)) return;

        const float current = m_smoothedValues[index];
        m_smoothedValues[index] = CalculateSmoothedValue(current, targetValue);
    }

    float LedPanelRenderer::CalculateSmoothedValue(
        float current,
        float target
    ) const
    {
        const float rate = GetSmoothingRate(current, target);
        return Helpers::Math::Lerp(current, target, rate);
    }

    float LedPanelRenderer::GetSmoothingRate(
        float current,
        float target
    ) const
    {
        const float baseRate = (current < target) ? kAttackRate : (1.0f - kDecayRate);
        return baseRate * m_settings.smoothingMultiplier;
    }

    size_t LedPanelRenderer::GetUpdateCount(const SpectrumData& spectrum) const
    {
        return std::min(
            static_cast<size_t>(m_grid.columns),
            spectrum.size()
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Layers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::RenderInactiveLeds(Canvas& canvas) const
    {
        const Color inactiveColor = GetInactiveColor();
        const Paint paint = Paint::Fill(inactiveColor);

        canvas.DrawCircleBatch(m_allLedPositions, kLedRadius, paint);
    }

    void LedPanelRenderer::RenderActiveLeds(Canvas& canvas) const
    {
        for (int col = 0; col < m_grid.columns; ++col) {
            RenderColumnLeds(canvas, col);
        }
    }

    void LedPanelRenderer::RenderPeakLeds(Canvas& canvas) const
    {
        for (int col = 0; col < m_grid.columns; ++col) {
            if (!m_peakTracker.IsPeakVisible(col)) continue;

            const int peakRow = CalculatePeakRow(m_peakTracker.GetPeak(col));

            if (IsPeakRowValid(peakRow)) {
                RenderPeakLed(canvas, col, peakRow);
            }
        }
    }

    void LedPanelRenderer::RenderColumnLeds(
        Canvas& canvas,
        int col
    ) const
    {
        const float value = m_smoothedValues[col];
        int activeLeds = CalculateActiveLedCount(value);

        if (ShouldRenderMinimumLed(value, activeLeds)) {
            activeLeds = 1;
        }

        const float brightness = CalculateBrightness(value);

        for (int row = 0; row < activeLeds; ++row) {
            const bool isTopLed = (row == activeLeds - 1);
            const float ledBrightness = CalculateLedBrightness(brightness, isTopLed);

            const Color ledColor = GetLedColor(row, Saturate(ledBrightness));
            const size_t ledIndex = GetLedIndex(col, row);

            RenderSingleLed(canvas, ledIndex, ledColor);
        }
    }

    void LedPanelRenderer::RenderSingleLed(
        Canvas& canvas,
        size_t ledIndex,
        const Color& color
    ) const
    {
        if (!IsLedIndexValid(ledIndex)) return;

        const Paint paint = Paint::Fill(color);
        canvas.DrawCircle(m_allLedPositions[ledIndex], kLedRadius, paint);
    }

    void LedPanelRenderer::RenderPeakLed(
        Canvas& canvas,
        int col,
        int peakRow
    ) const
    {
        const size_t ledIndex = GetLedIndex(col, peakRow);

        if (!IsLedIndexValid(ledIndex)) return;

        const Color peakColor = GetPeakColor();
        const Paint paint = Paint::Stroke(peakColor, kPeakStrokeWidth);

        canvas.DrawCircle(
            m_allLedPositions[ledIndex],
            kLedRadius + kPeakRadiusOffset,
            paint
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    int LedPanelRenderer::CalculateActiveLedCount(float value) const
    {
        return static_cast<int>(value * m_grid.rows);
    }

    int LedPanelRenderer::CalculatePeakRow(float peakValue) const
    {
        return static_cast<int>(peakValue * m_grid.rows) - 1;
    }

    size_t LedPanelRenderer::GetLedIndex(
        int col,
        int row
    ) const
    {
        return static_cast<size_t>(col) * m_grid.rows + row;
    }

    Point LedPanelRenderer::GetGridCenter() const
    {
        const float gridWidth = m_grid.columns * m_grid.cellSize;
        const float gridHeight = m_grid.rows * m_grid.cellSize;

        const Point gridSize = { gridWidth, gridHeight };
        const Point halfSize = Multiply(gridSize, 0.5f);

        return Add(m_grid.gridStart, halfSize);
    }

    Rect LedPanelRenderer::GetGridBounds() const
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
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color LedPanelRenderer::GetLedColor(
        int row,
        float brightness
    ) const
    {
        Color baseColor = GetRowBaseColor(row);

        if (HasExternalColor()) {
            const float t = GetColorBlendRatio(row);
            baseColor = BlendWithExternalColor(baseColor, t);
        }

        return ApplyBrightness(baseColor, brightness);
    }

    Color LedPanelRenderer::GetRowBaseColor(int row) const
    {
        const int rowIndex = Helpers::Math::Clamp(row, 0, static_cast<int>(m_rowColors.size()) - 1);

        if (!IsRowIndexValid(rowIndex)) return {};

        return m_rowColors[rowIndex];
    }

    Color LedPanelRenderer::ApplyBrightness(
        Color color,
        float brightness
    ) const
    {
        color.a = brightness;
        return color;
    }

    Color LedPanelRenderer::GetInactiveColor() const
    {
        Color color = kInactiveColorBase;
        color.a = kInactiveAlpha;

        if (m_isOverlay) {
            color.a *= kOverlayPaddingFactor;
        }

        return color;
    }

    Color LedPanelRenderer::GetPeakColor() const
    {
        return kPeakColorBase;
    }

    float LedPanelRenderer::CalculateBrightness(float value) const
    {
        return Helpers::Math::Lerp(kMinActiveBrightness, 1.0f, value);
    }

    float LedPanelRenderer::CalculateLedBrightness(
        float baseBrightness,
        bool isTopLed
    ) const
    {
        return isTopLed
            ? baseBrightness * kTopLedBrightnessBoost
            : baseBrightness;
    }

    bool LedPanelRenderer::HasExternalColor() const
    {
        return (m_primaryColor.r != 1.0f ||
            m_primaryColor.g != 1.0f ||
            m_primaryColor.b != 1.0f);
    }

    Color LedPanelRenderer::BlendWithExternalColor(
        Color baseColor,
        float t
    ) const
    {
        const auto blend = [&](float ext, float grad, float t_) {
            return ext * kExternalColorBlend +
                grad * (1.0f - kExternalColorBlend) * t_;
            };

        return Color(
            blend(m_primaryColor.r, baseColor.r, t),
            blend(m_primaryColor.g, baseColor.g, t),
            blend(m_primaryColor.b, baseColor.b, t)
        );
    }

    float LedPanelRenderer::GetColorBlendRatio(int row) const
    {
        return (m_rowColors.size() > 1)
            ? static_cast<float>(row) / (m_rowColors.size() - 1)
            : 0.0f;
    }

    Color LedPanelRenderer::InterpolateGradient(float t)
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
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool LedPanelRenderer::IsGridValid() const
    {
        return m_grid.columns > 0 && m_grid.rows > 0;
    }

    bool LedPanelRenderer::CanUpdateGrid(size_t requiredColumns) const
    {
        return IsValidViewportSize() && requiredColumns > 0;
    }

    bool LedPanelRenderer::ShouldRecreateGrid(size_t requiredColumns) const
    {
        return !(IsGridValid() && static_cast<size_t>(m_grid.columns) >= requiredColumns);
    }

    bool LedPanelRenderer::IsValidViewportSize() const
    {
        return m_width > 0 && m_height > 0;
    }

    bool LedPanelRenderer::IsColumnIndexValid(size_t index) const
    {
        return index < m_smoothedValues.size();
    }

    bool LedPanelRenderer::IsRowIndexValid(int row) const
    {
        return row >= 0 && row < static_cast<int>(m_rowColors.size());
    }

    bool LedPanelRenderer::IsLedIndexValid(size_t index) const
    {
        return index < m_allLedPositions.size();
    }

    bool LedPanelRenderer::ShouldRenderMinimumLed(
        float value,
        int activeLeds
    ) const
    {
        return activeLeds == 0 && value > kMinValueThreshold;
    }

    bool LedPanelRenderer::IsPeakRowValid(int row) const
    {
        return row >= 0 && row < m_grid.rows;
    }

} // namespace Spectrum