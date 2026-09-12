#ifndef SPECTRUM_CPP_LED_PANEL_RENDERER_H
#define SPECTRUM_CPP_LED_PANEL_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// LedPanelRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class LedPanelRenderer final : public BaseRenderer<LedPanelRenderer> {
    public:
        LedPanelRenderer() { InitializePeakTracker(0, 0.5f, 0.95f); UpdateSettings(); }

        [[nodiscard]] std::string_view GetName() const override { return "LED Panel"; }

        void OnActivate(int width, int height) override {
            BaseRenderer::OnActivate(width, height);
            m_grid = {};
        }

    protected:
        void OnSettingsUpdated() override { m_grid = {}; m_gradient = RenderUtils::LedGradient(); }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            SyncGrid(m_grid, spectrum.size(), m_settings.maxRows);
            if (m_settings.usePeakHold && HasPeakTracker())
                GetPeakTracker().Update(spectrum, dt);
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (m_grid.columns == 0 || m_grid.rows == 0 || spectrum.empty()) return;

            const float radius = (std::min(m_grid.cellW, m_grid.cellH) - kMargin) * 0.5f;

            PointBatch inactive;
            RenderUtils::FillIdleGrid(
                inactive, m_grid.columns, m_grid.rows, RenderUtils::LedIdleColor(),
                [&](int col, int row) { return LedCenter(col, row); });
            RenderCircleBatches(ctx, inactive, radius);

            PointBatch active;
            const size_t cols = std::min(size_t(m_grid.columns), spectrum.size());
            for (size_t col = 0; col < cols; ++col) {
                const float mag = Normalized(spectrum[col]);
                RenderUtils::ForEachLitLed(mag, m_grid.rows, false, [&](int row, int lit) {
                    active[LedColor(
                        RenderUtils::RowT(row, m_grid.rows),
                        RenderUtils::LedBrightness(mag, row == lit - 1))]
                        .push_back(LedCenter(int(col), row));
                    });
            }
            RenderCircleBatches(ctx, active, radius);

            if (!m_settings.usePeakHold || !HasPeakTracker()) return;

            const auto& peaks = GetPeakTracker();
            const Color peakColor = AdjustAlpha(Color::White(), kPeakAlpha);
            for (size_t col = 0; col < cols; ++col) {
                if (!peaks.IsPeakVisible(col)) continue;
                const int row = RenderUtils::PeakRow(peaks.GetPeak(col), m_grid.rows);
                if (!RenderUtils::IsValidRow(row, m_grid.rows)) continue;
                Draw::StrokeCircle(
                    ctx, LedCenter(int(col), row),
                    radius + kPeakStroke, peakColor, kPeakStroke);
            }
        }

    private:
        static constexpr float kMargin = 3.0f;
        static constexpr float kPeakStroke = 2.0f;
        static constexpr float kPeakAlpha = 0.8f;

        [[nodiscard]] Point LedCenter(int col, int row) const {
            return GetGridCellCenter(m_grid, col, m_grid.rows - 1 - row);
        }

        [[nodiscard]] Color LedColor(float rowT, float brightness) const {
            return AdjustAlpha(
                RenderUtils::BlendLedPrimary(
                    SampleGradient(m_gradient, rowT), GetPrimaryColor(), rowT),
                brightness);
        }

        GridConfig m_grid{};
        ColorGradient m_gradient;
    };

} // namespace Spectrum

#endif