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

#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
#include <algorithm>

namespace Spectrum {

    using namespace D2DHelpers;

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

        constexpr float kCenterOffset = 0.5f;

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

        if (!IsGridValid()) return;

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
        m_peakValues.assign(m_grid.columns, 0.0f);
        m_peakTimers.assign(m_grid.columns, 0.0f);

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

        GridData grid;
        grid.rows = rows;
        grid.columns = cols;
        grid.cellSize = cellSize;
        grid.startX = GetGridStartX(gridWidth);
        grid.startY = GetGridStartY(gridHeight);

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
        const float halfCell = m_grid.cellSize * kCenterOffset;

        const float x = m_grid.startX + col * m_grid.cellSize + halfCell;
        const float y = m_grid.startY
            + (m_grid.rows - 1 - row) * m_grid.cellSize
            + halfCell;

        return { x, y };
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

    void LedPanelRenderer::UpdatePeakValues(float deltaTime)
    {
        for (int i = 0; i < m_grid.columns; ++i) {
            UpdatePeak(i, m_smoothedValues[i], deltaTime);
        }
    }

    void LedPanelRenderer::UpdatePeak(
        size_t index,
        float currentValue,
        float deltaTime
    )
    {
        if (!IsColumnIndexValid(index)) return;

        if (currentValue >= m_peakValues[index]) {
            m_peakValues[index] = currentValue;
            m_peakTimers[index] = kPeakHoldTime;
        }
        else if (m_peakTimers[index] > 0.0f) {
            UpdatePeakHoldTimer(index, deltaTime);
        }
        else {
            UpdatePeakDecay(index);
        }
    }

    void LedPanelRenderer::UpdatePeakHoldTimer(
        size_t index,
        float deltaTime
    )
    {
        m_peakTimers[index] -= deltaTime;
    }

    void LedPanelRenderer::UpdatePeakDecay(size_t index)
    {
        m_peakValues[index] *= kPeakDecayRate;
    }

    float LedPanelRenderer::CalculateSmoothedValue(
        float current,
        float target
    ) const
    {
        const float rate = GetSmoothingRate(current, target);
        return Utils::Lerp(current, target, rate);
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
            if (!IsPeakVisible(col)) continue;

            const int peakRow = CalculatePeakRow(m_peakValues[col]);

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

            const Color ledColor = GetLedColor(row, Utils::Saturate(ledBrightness));
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

    float LedPanelRenderer::GetGridStartX(float gridWidth) const
    {
        return (m_width - gridWidth) * kCenterOffset;
    }

    float LedPanelRenderer::GetGridStartY(float gridHeight) const
    {
        return (m_height - gridHeight) * kCenterOffset;
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
        const int rowIndex = std::min(row, static_cast<int>(m_rowColors.size()) - 1);

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
        return Utils::Lerp(kMinActiveBrightness, 1.0f, value);
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

        return Utils::InterpolateColor(
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

    bool LedPanelRenderer::IsPeakVisible(size_t index) const
    {
        return IsColumnIndexValid(index) && m_peakTimers[index] > 0.0f;
    }

    bool LedPanelRenderer::IsPeakRowValid(int row) const
    {
        return row >= 0 && row < m_grid.rows;
    }

} // namespace Spectrum