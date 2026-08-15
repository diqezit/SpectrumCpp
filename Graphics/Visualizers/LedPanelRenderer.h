#ifndef SPECTRUM_CPP_LED_PANEL_RENDERER_H
#define SPECTRUM_CPP_LED_PANEL_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// LedPanelRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class LedPanelRenderer final : public BaseRenderer<LedPanelRenderer> {
    public:
        LedPanelRenderer() {
            InitializePeakTracker(0, 0.5f, 0.95f);
            UpdateSettings();
        }

        [[nodiscard]] std::string_view GetName() const override { return "LED Panel"; }

        void OnActivate(int width, int height) override {
            BaseRenderer::OnActivate(width, height);
            m_grid = {};
        }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::LedPanelSettings>();
            m_grid = {};
            m_gradient = RenderUtils::LedGradient();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            SyncGrid(m_grid, spectrum.size(), kRadius * 2.0f + kMargin, m_settings.maxRows);
            if (m_settings.usePeakHold && HasPeakTracker())
                GetPeakTracker().Update(spectrum, dt);
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (m_grid.columns == 0 || m_grid.rows == 0 || spectrum.empty()) return;

            const Color idle = RenderUtils::LedIdleColor();
            PointBatch inactive;
            PointBatch active;

            for (int col = 0; col < m_grid.columns; ++col)
                for (int row = 0; row < m_grid.rows; ++row)
                    inactive[idle].push_back(LedCenter(col, row));
            RenderCircleBatches(ctx, inactive, kRadius);

            const size_t cols = std::min(static_cast<size_t>(m_grid.columns), spectrum.size());
            for (size_t col = 0; col < cols; ++col) {
                const float mag = Helpers::Sanitize::Normalized(spectrum[col]);
                const int lit = RenderUtils::LitRows(mag, m_grid.rows);
                if (lit == 0) continue;

                for (int row = 0; row < lit; ++row) {
                    active[LedColor(
                        RenderUtils::RowT(row, m_grid.rows),
                        RenderUtils::LedBrightness(mag, row == lit - 1))]
                        .push_back(LedCenter(static_cast<int>(col), row));
                }
            }
            RenderCircleBatches(ctx, active, kRadius);

            if (!m_settings.usePeakHold || !HasPeakTracker()) return;

            const auto& peaks = GetPeakTracker();
            for (size_t col = 0; col < cols; ++col) {
                if (!peaks.IsPeakVisible(col)) continue;
                const int row = RenderUtils::LitRows(peaks.GetPeak(col), m_grid.rows) - 1;
                if (row < 0 || row >= m_grid.rows) continue;
                Draw::StrokeCircle(
                    ctx, LedCenter(static_cast<int>(col), row),
                    kRadius + kPeakStroke,
                    AdjustAlpha(Color::White(), kPeakAlpha),
                    kPeakStroke);
            }
        }

    private:
        static constexpr float kRadius = 6.0f;
        static constexpr float kMargin = 3.0f;
        static constexpr float kPeakStroke = 2.0f;
        static constexpr float kPeakAlpha = 0.8f;
        static constexpr float kBlend = 0.7f;

        [[nodiscard]] Point LedCenter(int col, int row) const {
            return GetGridCellCenter(m_grid, col, m_grid.rows - 1 - row);
        }

        [[nodiscard]] Color LedColor(float rowT, float brightness) const {
            Color color = SampleGradient(m_gradient, rowT);
            const Color& primary = GetPrimaryColor();
            if (primary.r != 1.0f || primary.g != 1.0f || primary.b != 1.0f)
                color = InterpolateColor(primary, color, rowT * (1.0f - kBlend) + kBlend);
            return AdjustAlpha(color, brightness);
        }

        Settings::LedPanelSettings m_settings{};
        GridConfig m_grid{};
        ColorGradient m_gradient;
    };

} // namespace Spectrum

#endif