#ifndef SPECTRUM_CPP_MATRIX_LED_RENDERER_H
#define SPECTRUM_CPP_MATRIX_LED_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// MatrixLedRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class MatrixLedRenderer final : public BaseRenderer<MatrixLedRenderer> {
    public:
        MatrixLedRenderer() {
            InitializePeakTracker(0, 0.5f, 0.95f);
            UpdateSettings();
        }

        [[nodiscard]] std::string_view GetName() const override { return "Matrix LED"; }

        void OnActivate(int width, int height) override {
            BaseRenderer::OnActivate(width, height);
            m_grid = {};
        }

    protected:
        void OnSettingsUpdated() override {
            m_grid = {};
            m_gradient = RenderUtils::LedGradient();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            SyncGrid(m_grid, spectrum.size(), kSize + kMargin, m_settings.ledDensity);
            if (m_settings.enableGlow && HasPeakTracker())
                GetPeakTracker().Update(spectrum, dt);
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (m_grid.columns == 0 || m_grid.rows == 0 || spectrum.empty()) return;

            RectBatch inactive;
            RenderUtils::FillIdleGrid(
                inactive, m_grid.columns, m_grid.rows, RenderUtils::LedIdleColor(IsOverlay()),
                [&](int col, int row) { return LedRect(col, row); });
            RenderRectBatches(ctx, inactive);

            RectBatch active;
            const size_t cols = std::min(size_t(m_grid.columns), spectrum.size());
            for (size_t col = 0; col < cols; ++col) {
                const float mag = Normalized(spectrum[col]);
                RenderUtils::ForEachLitLed(mag, m_grid.rows, true, [&](int row, int lit) {
                    active[RenderUtils::LedColor(
                        m_gradient,
                        RenderUtils::RowT(row, m_grid.rows),
                        RenderUtils::LedBrightness(mag, row == lit - 1))]
                        .push_back(LedRect(int(col), row));
                    });
            }
            RenderRectBatches(ctx, active);

            if (!m_settings.enableGlow || !HasPeakTracker()) return;

            RectBatch peaks;
            const auto& tracker = GetPeakTracker();
            const Color peakColor = AdjustAlpha(Color::White(), IsOverlay() ? 0.76f : 0.8f);
            for (size_t col = 0; col < cols; ++col) {
                if (!tracker.IsPeakVisible(col)) continue;
                const int row = RenderUtils::PeakRow(tracker.GetPeak(col), m_grid.rows);
                if (!RenderUtils::IsValidRow(row, m_grid.rows)) continue;
                peaks[peakColor].push_back(LedRect(int(col), row));
            }
            RenderRectBatches(ctx, peaks);
        }

    private:
        static constexpr float kSize = 4.0f;
        static constexpr float kMargin = 1.0f;

        [[nodiscard]] Rect LedRect(int col, int row) const {
            const Point c = GetGridCellCenter(m_grid, col, m_grid.rows - 1 - row);
            return { c.x - kSize * 0.5f, c.y - kSize * 0.5f, kSize, kSize };
        }

        GridConfig m_grid{};
        ColorGradient m_gradient;
    };

} // namespace Spectrum

#endif