#ifndef SPECTRUM_CPP_MATRIX_LED_RENDERER_H
#define SPECTRUM_CPP_MATRIX_LED_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class Canvas;

    class MatrixLedRenderer final : public BaseRenderer<MatrixLedRenderer>
    {
    public:
        MatrixLedRenderer();
        ~MatrixLedRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::MatrixLed;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Matrix LED";
        }

        void OnActivate(int width, int height) override;

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        using Settings = Settings::MatrixLedSettings;

        static constexpr float kLedSize = 4.0f;
        static constexpr float kLedMargin = 1.0f;
        static constexpr float kInactiveAlpha = 0.08f;
        static constexpr float kMinActiveBrightness = 0.4f;
        static constexpr float kHeightScale = 0.95f;
        static constexpr float kMinMagnitudeThreshold = 0.05f;
        static constexpr float kTopLedBoost = 1.2f;
        static constexpr float kOverlayAlphaScale = 0.95f;
        static constexpr float kPeakAlpha = 0.8f;
        static constexpr float kPeakOverlayAlpha = 0.76f;

        void UpdateGridConfiguration(size_t requiredColumns);

        void RenderInactiveLeds(Canvas& canvas);

        void RenderActiveLeds(
            Canvas& canvas,
            const SpectrumData& spectrum
        );

        void RenderPeakIndicators(
            Canvas& canvas,
            size_t columnCount
        );

        [[nodiscard]] Rect GetLedRect(
            int column,
            int row
        ) const;

        [[nodiscard]] Color CalculateLedColor(
            int row,
            float brightness,
            bool isTopLed
        ) const;

        [[nodiscard]] int CalculateActiveLeds(
            float magnitude
        ) const;

        Settings m_settings;
        GridConfig m_grid;
        ColorGradient m_gradient;
    };

} // namespace Spectrum

#endif