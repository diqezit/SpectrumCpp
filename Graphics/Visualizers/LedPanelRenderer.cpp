#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"

namespace Spectrum {

    using namespace Helpers::Math;
    using namespace Helpers::Sanitize;

    LedPanelRenderer::LedPanelRenderer() {
        InitializePeakTracker(0, 0.5f, 0.95f);
        UpdateSettings();
    }

    void LedPanelRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        m_grid = {};
    }

    void LedPanelRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
        m_grid = {};

        m_gradient = {
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
    }

    void LedPanelRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        UpdateGridConfiguration(spectrum.size());

        if (m_settings.usePeakHold && HasPeakTracker()) {
            GetPeakTracker().Update(spectrum, deltaTime);
        }
    }

    void LedPanelRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        if (m_grid.columns == 0 ||
            m_grid.rows == 0 ||
            spectrum.empty()) {
            return;
        }

        RenderInactiveLeds(canvas);
        RenderActiveLeds(canvas, spectrum);

        if (m_settings.usePeakHold && HasPeakTracker()) {
            const size_t columnCount = std::min(
                static_cast<size_t>(m_grid.columns),
                spectrum.size()
            );
            RenderPeakIndicators(canvas, columnCount);
        }
    }

    void LedPanelRenderer::UpdateGridConfiguration(
        size_t requiredColumns
    ) {
        const float cellSize = kLedRadius * 2.0f + kLedMargin;
        auto newGrid = CalculateGrid(
            requiredColumns,
            cellSize,
            m_settings.maxRows
        );

        if (m_grid.columns != newGrid.columns ||
            m_grid.rows != newGrid.rows) {
            m_grid = newGrid;

            if (HasPeakTracker()) {
                GetPeakTracker().Resize(m_grid.columns);
            }
        }
    }

    void LedPanelRenderer::RenderInactiveLeds(Canvas& canvas) {
        std::vector<Point> inactivePositions;
        inactivePositions.reserve(
            static_cast<size_t>(m_grid.columns) * m_grid.rows
        );

        for (int col = 0; col < m_grid.columns; ++col) {
            for (int row = 0; row < m_grid.rows; ++row) {
                inactivePositions.push_back(
                    GetLedCenter(col, row)
                );
            }
        }

        const Color inactiveColor = AdjustAlpha(
            Color::FromRGB(80, 80, 80),
            kInactiveAlpha
        );

        canvas.DrawCircleBatch(
            inactivePositions,
            kLedRadius,
            Paint::Fill(inactiveColor)
        );
    }

    void LedPanelRenderer::RenderActiveLeds(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        PointBatch activeBatches;
        const size_t columnCount = std::min(
            static_cast<size_t>(m_grid.columns),
            spectrum.size()
        );

        for (size_t col = 0; col < columnCount; ++col) {
            const float magnitude = NormalizedFloat(spectrum[col]);
            const int activeLeds = CalculateActiveLeds(magnitude);

            if (activeLeds == 0) continue;

            const float brightness = Lerp(
                kMinActiveBrightness,
                1.0f,
                magnitude
            );

            for (int row = 0; row < activeLeds; ++row) {
                const float rowNorm = static_cast<float>(row) /
                    std::max(1, m_grid.rows - 1);

                const bool isTopLed = (row == activeLeds - 1);
                const Color ledColor = CalculateLedColor(
                    rowNorm,
                    brightness,
                    isTopLed
                );

                const Point center = GetLedCenter(
                    static_cast<int>(col),
                    row
                );

                activeBatches[ledColor].push_back(center);
            }
        }

        RenderCircleBatches(canvas, activeBatches, kLedRadius);
    }

    void LedPanelRenderer::RenderPeakIndicators(
        Canvas& canvas,
        size_t columnCount
    ) {
        const auto& tracker = GetPeakTracker();

        for (size_t col = 0; col < columnCount; ++col) {
            if (!tracker.IsPeakVisible(col)) continue;

            const float peakHeight = RenderUtils::MagnitudeToHeight(
                tracker.GetPeak(col),
                1.0f,
                kHeightScale
            );

            const int peakRow = static_cast<int>(
                peakHeight * m_grid.rows
                ) - 1;

            if (peakRow >= 0 && peakRow < m_grid.rows) {
                const Point peakCenter = GetLedCenter(
                    static_cast<int>(col),
                    peakRow
                );

                canvas.DrawCircle(
                    peakCenter,
                    kLedRadius + kPeakStrokeWidth,
                    Paint::Stroke(
                        AdjustAlpha(Color::White(), kPeakAlpha),
                        kPeakStrokeWidth
                    )
                );
            }
        }
    }

    Point LedPanelRenderer::GetLedCenter(
        int col,
        int row
    ) const {
        return GetGridCellCenter(
            m_grid,
            col,
            m_grid.rows - 1 - row
        );
    }

    Color LedPanelRenderer::CalculateLedColor(
        float rowNorm,
        float brightness,
        bool isTopLed
    ) const {
        Color ledColor = SampleGradient(m_gradient, rowNorm);

        const Color& primary = GetPrimaryColor();
        const bool hasPrimaryColor = (
            primary.r != 1.0f ||
            primary.g != 1.0f ||
            primary.b != 1.0f
            );

        if (hasPrimaryColor) {
            const float blendFactor = rowNorm *
                (1.0f - kColorBlendAmount) + kColorBlendAmount;

            ledColor = InterpolateColors(
                primary,
                ledColor,
                blendFactor
            );
        }

        const float finalBrightness = isTopLed
            ? brightness * kTopLedBoost
            : brightness;

        return AdjustAlpha(ledColor, Saturate(finalBrightness));
    }

    int LedPanelRenderer::CalculateActiveLeds(
        float magnitude
    ) const {
        const float height = RenderUtils::MagnitudeToHeight(
            magnitude,
            1.0f,
            kHeightScale
        );

        return static_cast<int>(height * m_grid.rows);
    }

} // namespace Spectrum