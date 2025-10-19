// LedPanelRenderer.h

#ifndef SPECTRUM_CPP_LED_PANEL_RENDERER_H
#define SPECTRUM_CPP_LED_PANEL_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class LedPanelRenderer final : public BaseRenderer {
    public:
        LedPanelRenderer();
        ~LedPanelRenderer() override = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        RenderStyle GetStyle() const override {
            return RenderStyle::LedPanel;
        }

        std::string_view GetName() const override {
            return "LED Panel";
        }

        bool SupportsPrimaryColor() const override {
            return true;
        }

        void OnActivate(int width, int height) override;

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BaseRenderer Overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings & Data Structs
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings {
            bool usePeakHold;
            int maxRows;
            float smoothingMultiplier;
        };

        struct GridData {
            int rows = 0;
            int columns = 0;
            float cellSize = 0.0f;
            float startX = 0.0f;
            float startY = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateGrid(size_t requiredColumns);
        void CreateGrid(const GridData& gridData);
        void CacheLedPositions();
        void InitializeRowColors();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation & Value Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateValues(const SpectrumData& spectrum);
        void UpdatePeakValues(float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderInactiveLeds(GraphicsContext& context);
        void RenderActiveLeds(GraphicsContext& context);
        void RenderPeakLeds(GraphicsContext& context);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Color GetLedColor(int row, float brightness) const;
        bool HasExternalColor() const;

        Color BlendWithExternalColor(
            Color baseColor,
            float t
        ) const;

        static Color InterpolateGradient(float t);

        static bool ColorsAreSimilar(
            const Color& c1,
            const Color& c2,
            float threshold = 0.01f
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        GridData m_grid;

        std::vector<float> m_smoothedValues;
        std::vector<float> m_peakValues;
        std::vector<float> m_peakTimers;

        std::vector<Point> m_allLedPositions;
        std::vector<Color> m_rowColors;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_LED_PANEL_RENDERER_H