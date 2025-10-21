#ifndef SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H
#define SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the KenwoodBarsRenderer for Kenwood-style bar visualization.
//
// This renderer recreates the classic Kenwood stereo visualizer look with
// gradient bars that transition from red through yellow to green, plus
// sticky peak indicators that hold momentarily before falling.
//
// Key features:
// - Fixed gradient palette (red->yellow->green)
// - Sticky peak indicators with hold time
// - Quality-dependent outlines and enhanced peaks
// - Overlay mode support with adjusted transparency
// - Does not support primary color (uses fixed palette)
//
// Design notes:
// - Peak hold algorithm: peaks stick at max for PEAK_HOLD_TIME_S
// - Rendering layers: main bars -> outlines -> peaks -> enhancements
// - All rendering methods are const (peak state is mutable)
// - Gradient intensity boosted for vibrant Kenwood look
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BaseRenderer.h"
#include "RenderUtils.h"
#include "SpectrumTypes.h"

namespace Spectrum {

    class Canvas;

    class KenwoodBarsRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        KenwoodBarsRenderer();
        ~KenwoodBarsRenderer() override = default;

        KenwoodBarsRenderer(const KenwoodBarsRenderer&) = delete;
        KenwoodBarsRenderer& operator=(const KenwoodBarsRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::KenwoodBars; }
        [[nodiscard]] std::string_view GetName() const override { return "Kenwood Bars"; }
        [[nodiscard]] bool SupportsPrimaryColor() const override { return false; }

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
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings
        {
            bool useGradient;
            bool useRoundCorners;
            bool useOutline;
            bool useEnhancedPeaks;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Peak Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void EnsurePeakArraySize(size_t size);

        void UpdatePeak(
            size_t index,
            float value,
            float deltaTime
        );

        [[nodiscard]] float GetPeakValue(size_t index) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Style Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float CalculateCornerRadius(float barWidth) const;
        [[nodiscard]] BarStyle CreateBarStyle(float cornerRadius) const;
        [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> GetAdjustedGradientStops() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderMainLayer(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const RenderUtils::BarLayout& layout,
            const BarStyle& barStyle
        ) const;

        void RenderOutlineLayer(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const RenderUtils::BarLayout& layout,
            float cornerRadius
        ) const;

        void RenderPeakLayer(
            Canvas& canvas,
            const RenderUtils::BarLayout& layout,
            float cornerRadius
        ) const;

        void RenderPeakEnhancementLayer(
            Canvas& canvas,
            const RenderUtils::BarLayout& layout
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_currentSettings;
        mutable std::vector<float> m_peaks;
        mutable std::vector<float> m_peakTimers;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_KENWOOD_BARS_RENDERER_H