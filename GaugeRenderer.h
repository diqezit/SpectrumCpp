#ifndef SPECTRUM_CPP_GAUGE_RENDERER_H
#define SPECTRUM_CPP_GAUGE_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class GaugeRenderer final : public BaseRenderer {
    public:
        GaugeRenderer();
        ~GaugeRenderer() override = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        RenderStyle GetStyle() const override { return RenderStyle::Gauge; }
        std::string_view GetName() const override { return "Gauge"; }
        bool SupportsPrimaryColor() const override { return false; }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BaseRenderer Overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
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
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Drawing Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        void DrawBackground(GraphicsContext& context, const Rect& rect) const;
        void DrawScale(GraphicsContext& context, const Rect& rect) const;
        void DrawNeedle(GraphicsContext& context, const Rect& rect) const;
        void DrawPeakIndicator(GraphicsContext& context, const Rect& rect) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Scale Drawing Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        void DrawMajorTick(
            GraphicsContext& context,
            const Point& center,
            float radiusX,
            float radiusY,
            float value,
            const wchar_t* label
        ) const;

        void DrawMinorTick(
            GraphicsContext& context,
            const Point& center,
            float radiusX,
            float radiusY,
            float value
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Needle Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        void DrawNeedleBody(
            GraphicsContext& context,
            const Point& center,
            float length
        ) const;

        void DrawNeedlePivot(
            GraphicsContext& context,
            const Point& center,
            float radius
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        float CalculateLoudness(const SpectrumData& spectrum) const;
        float DbToAngle(float db) const;
        Point GetScaleCenter(const Rect& rect) const;
        Point GetNeedleCenter(const Rect& rect) const;
        float GetTickLength(float value, bool isMajor) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        float m_currentDbValue;
        float m_currentNeedleAngle;
        int m_peakHoldCounter;
        bool m_peakActive;

        float m_smoothingFactorInc;
        float m_smoothingFactorDec;
        float m_riseSpeed;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GAUGE_RENDERER_H