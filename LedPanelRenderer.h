// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// LedPanelRenderer.h: Renders spectrum as a classic LED panel meter.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_LED_PANEL_RENDERER_H
#define SPECTRUM_CPP_LED_PANEL_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class LedPanelRenderer final : public BaseRenderer {
    public:
        LedPanelRenderer();
        ~LedPanelRenderer() override = default;

        RenderStyle GetStyle() const override {
            return RenderStyle::LedPanel;
        }
        std::string_view GetName() const override {
            return "LED Panel";
        }
        bool SupportsPrimaryColor() const override {
            return false;
        }
        void SetPrimaryColor(const Color& color) override {
            (void)color;
        }

    protected:
        void UpdateSettings() override;
        void UpdateAnimation(const SpectrumData& spectrum,
            float deltaTime) override;
        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        void CreateGradient();
        void UpdateGrid(size_t barCount);
        Color GetLedColor(int row, int totalRows, float brightness) const;

        struct Settings {
            int rows;
            bool usePeakHold;
            float peakHoldTime;
            float ledRadiusRatio;
        };

        struct GridData {
            int rows;
            int cols;
            float cellSize;
            float startX;
            float startY;
        };

        Settings m_settings;
        GridData m_grid;

        std::vector<float> m_currentValues;
        std::vector<float> m_peakValues;
        std::vector<float> m_peakTimers;
        std::array<Color, 3> m_gradient;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_LED_PANEL_RENDERER_H