#include "Graphics/Visualizers/MatrixLedRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"

namespace Spectrum {

    using namespace Helpers::Math;
    using namespace Helpers::Sanitize;

    MatrixLedRenderer::MatrixLedRenderer() {
        InitializePeakTracker(0, 0.5f, 0.95f);
        UpdateSettings();
    }

    void MatrixLedRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        m_grid = {};
    }

    void MatrixLedRenderer::UpdateSettings() {
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

    void MatrixLedRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        UpdateGridConfiguration(spectrum.size());

        if (m_settings.enableGlow && HasPeakTracker()) {
            GetPeakTracker().Update(spectrum, deltaTime);
        }
    }

    void MatrixLedRenderer::DoRender(
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

        if (m_settings.enableGlow && HasPeakTracker()) {
            const size_t columnCount = std::min(
                static_cast<size_t>(m_grid.columns),
                spectrum.size()
            );
            RenderPeakIndicators(canvas, columnCount);
        }
    }

    void MatrixLedRenderer::UpdateGridConfiguration(
        size_t requiredColumns
    ) {
        const float cellSize = kLedSize + kLedMargin;
        auto newGrid = CalculateGrid(
            requiredColumns,
            cellSize,
            m_settings.ledDensity
        );

        if (m_grid.columns != newGrid.columns ||
            m_grid.rows != newGrid.rows) {
            m_grid = newGrid;

            if (HasPeakTracker()) {
                GetPeakTracker().Resize(m_grid.columns);
            }
        }
    }

    void MatrixLedRenderer::RenderInactiveLeds(Canvas& canvas) {
        std::vector<Rect> inactiveRects;
        inactiveRects.reserve(
            static_cast<size_t>(m_grid.columns) * m_grid.rows
        );

        for (int col = 0; col < m_grid.columns; ++col) {
            for (int row = 0; row < m_grid.rows; ++row) {
                inactiveRects.push_back(GetLedRect(col, row));
            }
        }

        Color inactiveColor = AdjustAlpha(
            Color::FromRGB(80, 80, 80),
            kInactiveAlpha
        );

        if (IsOverlay()) {
            inactiveColor = AdjustAlpha(
                inactiveColor,
                inactiveColor.a * kOverlayAlphaScale
            );
        }

        canvas.DrawRectangleBatch(
            inactiveRects,
            Paint::Fill(inactiveColor)
        );
    }

    void MatrixLedRenderer::RenderActiveLeds(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        RectBatch activeBatches;
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
                const bool isTopLed = (row == activeLeds - 1);
                const Color ledColor = CalculateLedColor(
                    row,
                    brightness,
                    isTopLed
                );

                activeBatches[ledColor].push_back(
                    GetLedRect(static_cast<int>(col), row)
                );
            }
        }

        RenderRectBatches(canvas, activeBatches);
    }

    void MatrixLedRenderer::RenderPeakIndicators(
        Canvas& canvas,
        size_t columnCount
    ) {
        std::vector<Rect> peakRects;
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
                peakRects.push_back(
                    GetLedRect(static_cast<int>(col), peakRow)
                );
            }
        }

        if (!peakRects.empty()) {
            const float alpha = IsOverlay()
                ? kPeakOverlayAlpha
                : kPeakAlpha;

            Color peakColor = AdjustAlpha(Color::White(), alpha);
            canvas.DrawRectangleBatch(
                peakRects,
                Paint::Fill(peakColor)
            );
        }
    }

    Rect MatrixLedRenderer::GetLedRect(
        int column,
        int row
    ) const {
        const Point center = GetGridCellCenter(
            m_grid,
            column,
            m_grid.rows - 1 - row
        );

        return {
            center.x - kLedSize * 0.5f,
            center.y - kLedSize * 0.5f,
            kLedSize,
            kLedSize
        };
    }

    Color MatrixLedRenderer::CalculateLedColor(
        int row,
        float brightness,
        bool isTopLed
    ) const {
        const float rowNorm = static_cast<float>(row) /
            std::max(1, m_grid.rows - 1);

        Color ledColor = SampleGradient(m_gradient, rowNorm);

        const float finalBrightness = isTopLed
            ? brightness * kTopLedBoost
            : brightness;

        return AdjustAlpha(ledColor, Saturate(finalBrightness));
    }

    int MatrixLedRenderer::CalculateActiveLeds(
        float magnitude
    ) const {
        const float height = RenderUtils::MagnitudeToHeight(
            magnitude,
            1.0f,
            kHeightScale
        );

        int activeLeds = static_cast<int>(height * m_grid.rows);

        if (activeLeds == 0 && magnitude > kMinMagnitudeThreshold) {
            activeLeds = 1;
        }

        return activeLeds;
    }

} // namespace Spectrum