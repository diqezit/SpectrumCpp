// LedPanelRenderer.cpp

#include "LedPanelRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr float LED_RADIUS = 6.0f;
        constexpr float LED_MARGIN = 3.0f;
        constexpr float INACTIVE_ALPHA = 0.08f;
        constexpr float MIN_ACTIVE_BRIGHTNESS = 0.4f;
        constexpr float DECAY_RATE = 0.85f;
        constexpr float ATTACK_RATE = 0.4f;
        constexpr float PEAK_HOLD_TIME = 0.5f;
        constexpr float OVERLAY_PADDING_FACTOR = 0.95f;
        constexpr float PEAK_STROKE_WIDTH = 2.0f;
        constexpr float PEAK_RADIUS_OFFSET = 2.0f;
        constexpr float PEAK_DECAY_RATE = 0.95f;
        constexpr float MIN_VALUE_THRESHOLD = 0.05f;
        constexpr float TOP_LED_BRIGHTNESS_BOOST = 1.2f;
        constexpr float EXTERNAL_COLOR_BLEND = 0.7f;

        constexpr int MIN_GRID_SIZE = 10;
        constexpr int MAX_COLUMNS = 64;

        const std::vector<Color> SPECTRUM_GRADIENT = {
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

        const Color INACTIVE_COLOR = Color::FromRGB(80, 80, 80);
        const Color PEAK_COLOR = Color(1.0f, 1.0f, 1.0f, 200.0f / 255.0f);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor & Settings
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LedPanelRenderer::LedPanelRenderer() {
        UpdateSettings();
    }

    void LedPanelRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        // Force grid recreation on activation or resize
        m_grid.columns = 0;
    }

    void LedPanelRenderer::UpdateSettings() {
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

        // Invalidate grid to force recreation with new settings
        m_grid.columns = 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Grid Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::UpdateGrid(size_t requiredColumns) {
        if (m_grid.columns > 0 &&
            static_cast<size_t>(m_grid.columns) >= requiredColumns) {
            return;
        }

        if (m_width <= 0 || m_height <= 0 || requiredColumns == 0) {
            return;
        }

        float ledSize = LED_RADIUS * 2.0f + LED_MARGIN;
        float availableWidth = m_isOverlay
            ? m_width * OVERLAY_PADDING_FACTOR
            : static_cast<float>(m_width);
        float availableHeight = m_isOverlay
            ? m_height * OVERLAY_PADDING_FACTOR
            : static_cast<float>(m_height);

        int cols = std::min({
            MAX_COLUMNS,
            static_cast<int>(requiredColumns),
            static_cast<int>(availableWidth / ledSize)
            });
        cols = std::max(MIN_GRID_SIZE, cols);

        int rows = std::min(
            m_settings.maxRows,
            static_cast<int>(availableHeight / ledSize)
        );
        rows = std::max(MIN_GRID_SIZE, rows);

        float cellSize = std::min(
            availableWidth / cols,
            availableHeight / rows
        );
        float gridWidth = cols * cellSize;
        float gridHeight = rows * cellSize;

        GridData newGrid;
        newGrid.rows = rows;
        newGrid.columns = cols;
        newGrid.cellSize = cellSize;
        newGrid.startX = (m_width - gridWidth) * 0.5f;
        newGrid.startY = (m_height - gridHeight) * 0.5f;

        CreateGrid(newGrid);
    }

    void LedPanelRenderer::CreateGrid(const GridData& gridData) {
        m_grid = gridData;

        m_smoothedValues.assign(m_grid.columns, 0.0f);
        m_peakValues.assign(m_grid.columns, 0.0f);
        m_peakTimers.assign(m_grid.columns, 0.0f);

        CacheLedPositions();
        InitializeRowColors();
    }

    void LedPanelRenderer::CacheLedPositions() {
        m_allLedPositions.clear();
        m_allLedPositions.reserve(
            static_cast<size_t>(m_grid.columns) * m_grid.rows
        );

        float halfCell = m_grid.cellSize * 0.5f;

        for (int col = 0; col < m_grid.columns; ++col) {
            float x = m_grid.startX + col * m_grid.cellSize + halfCell;

            for (int row = 0; row < m_grid.rows; ++row) {
                float y = m_grid.startY
                    + (m_grid.rows - 1 - row) * m_grid.cellSize
                    + halfCell;

                m_allLedPositions.push_back({ x, y });
            }
        }
    }

    void LedPanelRenderer::InitializeRowColors() {
        m_rowColors.resize(m_grid.rows);

        for (int i = 0; i < m_grid.rows; ++i) {
            float t = (m_grid.rows > 1)
                ? static_cast<float>(i) / (m_grid.rows - 1)
                : 0.0f;

            m_rowColors[i] = InterpolateGradient(t);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        UpdateGrid(spectrum.size());

        if (m_grid.columns == 0) {
            return;
        }

        UpdateValues(spectrum);

        if (m_settings.usePeakHold) {
            UpdatePeakValues(deltaTime);
        }
    }

    void LedPanelRenderer::UpdateValues(const SpectrumData& spectrum) {
        size_t count = std::min(
            static_cast<size_t>(m_grid.columns),
            spectrum.size()
        );

        for (size_t i = 0; i < count; ++i) {
            float current = m_smoothedValues[i];
            float target = spectrum[i];
            float rate = (current < target) ? ATTACK_RATE : (1.0f - DECAY_RATE);
            rate *= m_settings.smoothingMultiplier;

            m_smoothedValues[i] = Utils::Lerp(current, target, rate);
        }
    }

    void LedPanelRenderer::UpdatePeakValues(float deltaTime) {
        for (int i = 0; i < m_grid.columns; ++i) {
            if (m_smoothedValues[i] >= m_peakValues[i]) {
                m_peakValues[i] = m_smoothedValues[i];
                m_peakTimers[i] = PEAK_HOLD_TIME;
            }
            else if (m_peakTimers[i] > 0.0f) {
                m_peakTimers[i] -= deltaTime;
            }
            else {
                m_peakValues[i] *= PEAK_DECAY_RATE;
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void LedPanelRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        if (m_grid.columns == 0) {
            return;
        }

        RenderInactiveLeds(context);
        RenderActiveLeds(context);

        if (m_settings.usePeakHold) {
            RenderPeakLeds(context);
        }
    }

    void LedPanelRenderer::RenderInactiveLeds(GraphicsContext& context) {
        Color color = INACTIVE_COLOR;
        color.a = INACTIVE_ALPHA;

        if (m_isOverlay) {
            color.a *= OVERLAY_PADDING_FACTOR;
        }

        // Use batch rendering for all inactive LEDs
        context.DrawCircleBatch(
            m_allLedPositions,
            LED_RADIUS,
            color,
            true
        );
    }

    void LedPanelRenderer::RenderActiveLeds(GraphicsContext& context) {
        // Group LEDs by color for batch rendering
        struct LedBatch {
            Color color;
            std::vector<Point> positions;
        };

        std::vector<LedBatch> batches;
        batches.reserve(m_grid.rows); // One batch per row color at most

        for (int col = 0; col < m_grid.columns; ++col) {
            float value = m_smoothedValues[col];
            int activeLeds = static_cast<int>(value * m_grid.rows);

            if (activeLeds == 0 && value > MIN_VALUE_THRESHOLD) {
                activeLeds = 1;
            }

            float brightness = Utils::Lerp(MIN_ACTIVE_BRIGHTNESS, 1.0f, value);

            for (int row = 0; row < activeLeds; ++row) {
                bool isTopLed = (row == activeLeds - 1);
                float ledBrightness = isTopLed
                    ? brightness * TOP_LED_BRIGHTNESS_BOOST
                    : brightness;

                Color ledColor = GetLedColor(row, Utils::Saturate(ledBrightness));
                size_t ledIndex = static_cast<size_t>(col) * m_grid.rows + row;

                // Try to batch with existing color
                bool batched = false;
                for (auto& batch : batches) {
                    if (ColorsAreSimilar(batch.color, ledColor)) {
                        batch.positions.push_back(m_allLedPositions[ledIndex]);
                        batched = true;
                        break;
                    }
                }

                if (!batched) {
                    LedBatch newBatch;
                    newBatch.color = ledColor;
                    newBatch.positions.push_back(m_allLedPositions[ledIndex]);
                    batches.push_back(std::move(newBatch));
                }
            }
        }

        // Render all batches
        for (const auto& batch : batches) {
            if (!batch.positions.empty()) {
                context.DrawCircleBatch(
                    batch.positions,
                    LED_RADIUS,
                    batch.color,
                    true
                );
            }
        }
    }

    void LedPanelRenderer::RenderPeakLeds(GraphicsContext& context) {
        for (int col = 0; col < m_grid.columns; ++col) {
            if (m_peakTimers[col] <= 0.0f) {
                continue;
            }

            int peakRow = static_cast<int>(m_peakValues[col] * m_grid.rows) - 1;

            if (peakRow >= 0 && peakRow < m_grid.rows) {
                size_t ledIndex = static_cast<size_t>(col) * m_grid.rows + peakRow;

                context.DrawCircle(
                    m_allLedPositions[ledIndex],
                    LED_RADIUS + PEAK_RADIUS_OFFSET,
                    PEAK_COLOR,
                    false,
                    PEAK_STROKE_WIDTH
                );
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color LedPanelRenderer::GetLedColor(int row, float brightness) const {
        int rowIndex = std::min(row, static_cast<int>(m_rowColors.size()) - 1);

        if (rowIndex < 0) {
            return {};
        }

        Color baseColor = m_rowColors[rowIndex];

        // Check if external color should be applied
        if (HasExternalColor()) {
            float t = (m_rowColors.size() > 1)
                ? static_cast<float>(row) / (m_rowColors.size() - 1)
                : 0.0f;

            baseColor = BlendWithExternalColor(baseColor, t);
        }

        baseColor.a = brightness;
        return baseColor;
    }

    bool LedPanelRenderer::HasExternalColor() const {
        return (m_primaryColor.r != 1.0f ||
            m_primaryColor.g != 1.0f ||
            m_primaryColor.b != 1.0f);
    }

    Color LedPanelRenderer::BlendWithExternalColor(
        Color baseColor,
        float t
    ) const {
        auto blend = [](float ext, float grad, float t_) {
            return ext * EXTERNAL_COLOR_BLEND +
                grad * (1.0f - EXTERNAL_COLOR_BLEND) * t_;
            };

        return Color(
            blend(m_primaryColor.r, baseColor.r, t),
            blend(m_primaryColor.g, baseColor.g, t),
            blend(m_primaryColor.b, baseColor.b, t)
        );
    }

    Color LedPanelRenderer::InterpolateGradient(float t) {
        float scaledT = t * (SPECTRUM_GRADIENT.size() - 1);
        int index = static_cast<int>(scaledT);
        float fraction = scaledT - index;

        if (index >= static_cast<int>(SPECTRUM_GRADIENT.size()) - 1) {
            return SPECTRUM_GRADIENT.back();
        }

        return Utils::InterpolateColor(
            SPECTRUM_GRADIENT[index],
            SPECTRUM_GRADIENT[index + 1],
            fraction
        );
    }

    bool LedPanelRenderer::ColorsAreSimilar(
        const Color& c1,
        const Color& c2,
        float threshold
    ) {
        return (fabsf(c1.r - c2.r) < threshold &&
            fabsf(c1.g - c2.g) < threshold &&
            fabsf(c1.b - c2.b) < threshold &&
            fabsf(c1.a - c2.a) < threshold);
    }

} // namespace Spectrum