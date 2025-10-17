#ifndef SPECTRUM_CPP_GAUGE_RENDERER_H
#define SPECTRUM_CPP_GAUGE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the GaugeRenderer, a vintage VU meter style visualizer.
//
// This renderer displays audio loudness as an analog needle gauge with
// calibrated dB scale (-30dB to +5dB), peak indicator lamp, and realistic
// ballistics matching physical VU meter behavior.
//
// Key features:
// - RMS-based loudness calculation for perceived volume
// - Asymmetric needle response (fast attack, slow decay)
// - Peak lamp with hold time (matches hardware meters)
// - Vintage aesthetic with gradients and shadows
//
// Design notes:
// - All rendering methods are const (state in m_currentDbValue, etc.)
// - Quality settings control smoothing and visual effects
// - No external color support (fixed vintage color scheme)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include <utility>
#include <vector>

namespace Spectrum {

    class Canvas;

    class GaugeRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        GaugeRenderer();
        ~GaugeRenderer() override = default;

        GaugeRenderer(const GaugeRenderer&) = delete;
        GaugeRenderer& operator=(const GaugeRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::Gauge; }
        [[nodiscard]] std::string_view GetName() const override { return "Gauge"; }
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
        // Main Drawing Components (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Background Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawBezelLayers(
            Canvas& canvas,
            const Rect& rect
        ) const;

        void DrawMeterFace(
            Canvas& canvas,
            const Rect& outerRect
        ) const;

        void DrawVULabel(
            Canvas& canvas,
            const Rect& faceRect,
            float textSize
        ) const;

        [[nodiscard]] Rect GetInnerRect(const Rect& rect) const;
        [[nodiscard]] Rect GetFaceRect(const Rect& innerRect) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Scale Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawMajorTick(
            Canvas& canvas,
            const Point& center,
            float radiusX,
            float radiusY,
            float dbValue,
            const wchar_t* label
        ) const;

        void DrawMinorTick(
            Canvas& canvas,
            const Point& center,
            float radiusX,
            float radiusY,
            float dbValue
        ) const;

        void DrawTickLine(
            Canvas& canvas,
            const Point& start,
            const Point& end,
            const Color& color,
            float width
        ) const;

        void DrawTickLabel(
            Canvas& canvas,
            const Point& labelPos,
            float textSize,
            const wchar_t* label,
            const Color& color
        ) const;

        [[nodiscard]] std::pair<Point, Point> GetTickPoints(
            const Point& center,
            float radiusX,
            float radiusY,
            float angle,
            float tickLength
        ) const;

        [[nodiscard]] Color GetTickColor(
            float dbValue,
            bool isMinor
        ) const;

        [[nodiscard]] float GetLabelTextSize(
            const Rect& rect,
            float dbValue
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Needle Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawNeedleBody(
            Canvas& canvas,
            const Point& center,
            float length
        ) const;

        void DrawNeedlePivot(
            Canvas& canvas,
            const Point& center,
            float radius
        ) const;

        [[nodiscard]] std::vector<Point> GetNeedleGeometry(float length) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Peak Indicator Components
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawPeakLamp(
            Canvas& canvas,
            const Point& lampPos,
            float lampRadius
        ) const;

        void DrawPeakLabel(
            Canvas& canvas,
            const Point& lampPos,
            float lampRadius
        ) const;

        [[nodiscard]] Point GetPeakLampPosition(
            const Rect& rect,
            float lampRadius
        ) const;

        [[nodiscard]] Color GetPeakLampColor() const;
        [[nodiscard]] Color GetPeakTextColor() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float CalculateLoudness(const SpectrumData& spectrum) const;
        [[nodiscard]] float DbToAngle(float db) const;
        [[nodiscard]] Point GetScaleCenter(const Rect& rect) const;
        [[nodiscard]] Point GetNeedleCenter(const Rect& rect) const;

        [[nodiscard]] float GetTickLength(
            float dbValue,
            bool isMajor
        ) const;

        [[nodiscard]] Rect CreateCenteredTextRect(
            const Point& center,
            float width,
            float height
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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