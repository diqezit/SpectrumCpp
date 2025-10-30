#ifndef SPECTRUM_CPP_GAUGE_RENDERER_H
#define SPECTRUM_CPP_GAUGE_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class GaugeRenderer : public BaseRenderer<GaugeRenderer>
    {
    public:
        GaugeRenderer();
        ~GaugeRenderer() override = default;

        // IRenderer interface
        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Gauge;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Gauge";
        }

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
        void DrawBackground(
            Canvas& canvas,
            const Rect& rect
        ) const;

        void DrawScale(
            Canvas& canvas,
            const Rect& rect
        ) const;

        void DrawNeedle(
            Canvas& canvas,
            const Rect& rect
        ) const;

        void DrawPeakIndicator(
            Canvas& canvas,
            const Rect& rect
        ) const;

        [[nodiscard]] float CalculateLoudness(
            const SpectrumData& spectrum
        ) const;

        [[nodiscard]] float DbToAngle(float db) const;
        [[nodiscard]] Point GetNeedleCenter(const Rect& rect) const;
        [[nodiscard]] Rect CalculatePaddedRect() const;

        Settings::GaugeSettings m_settings;
        float m_currentDbValue;
        float m_currentNeedleAngle;
        int m_peakHoldCounter;
        bool m_peakActive;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GAUGE_RENDERER_H