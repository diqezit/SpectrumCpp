#ifndef SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H
#define SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H

#include "BaseRenderer.h"
#include "RenderUtils.h"
#include "SpectrumTypes.h"

namespace Spectrum {

    class KenwoodBarsRenderer final : public BaseRenderer {
    public:
        KenwoodBarsRenderer();
        ~KenwoodBarsRenderer() override = default;

        RenderStyle GetStyle() const override {
            return RenderStyle::KenwoodBars;
        }

        std::string_view GetName() const override {
            return "Kenwood Bars";
        }

        bool SupportsPrimaryColor() const override {
            return false;
        }

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
        // Settings & Helper Structs
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings {
            bool useGradient;
            bool useRoundCorners;
            bool useOutline;
            bool useEnhancedPeaks;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helper Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void EnsurePeakArraySize(size_t size);

        void UpdatePeak(
            size_t index,
            float value,
            float deltaTime
        );

        float GetPeakValue(size_t index) const;

        float CalculateCornerRadius(float barWidth) const;

        BarStyle CreateBarStyle(float cornerRadius) const;

        std::vector<D2D1_GRADIENT_STOP> GetAdjustedGradientStops() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderMainLayer(
            GraphicsContext& context,
            const SpectrumData& spectrum,
            const RenderUtils::BarLayout& layout,
            const BarStyle& barStyle
        ) const;

        void RenderOutlineLayer(
            GraphicsContext& context,
            const SpectrumData& spectrum,
            const RenderUtils::BarLayout& layout,
            float cornerRadius
        ) const;

        void RenderPeakLayer(
            GraphicsContext& context,
            const RenderUtils::BarLayout& layout,
            float cornerRadius
        ) const;

        void RenderPeakEnhancementLayer(
            GraphicsContext& context,
            const RenderUtils::BarLayout& layout
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_currentSettings;
        std::vector<float> m_peaks;
        std::vector<float> m_peakTimers;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H