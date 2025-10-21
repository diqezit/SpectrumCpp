// CircularWaveRenderer.h
#ifndef SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H
#define SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the CircularWaveRenderer for concentric ring visualizations.
//
// This renderer displays spectrum data as animated concentric rings that
// pulse and rotate based on audio intensity. Each ring represents a
// frequency band, with visual properties (radius, stroke, glow) driven
// by magnitude.
//
// Key features:
// - Dynamic rotation speed based on audio intensity
// - Sine wave animation for fluid ring motion
// - Magnitude-based glow effects (quality-dependent)
// - Back-to-front rendering for correct alpha blending
//
// Design notes:
// - All rendering methods are const (animation state in m_angle, m_waveTime)
// - Quality settings control ring count, glow, and stroke thickness
// - Uses RenderUtils for frequency band analysis
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BaseRenderer.h"

namespace Spectrum {

    class CircularWaveRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        CircularWaveRenderer();
        ~CircularWaveRenderer() override = default;

        CircularWaveRenderer(const CircularWaveRenderer&) = delete;
        CircularWaveRenderer& operator=(const CircularWaveRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::CircularWave; }
        [[nodiscard]] std::string_view GetName() const override { return "Circular Wave"; }

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
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct QualitySettings
        {
            bool useGlow;
            float maxStroke;
            int maxRings;
            float rotationSpeed;
            float waveSpeed;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Single Ring Rendering (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderCalculatedRing(
            GraphicsContext& context,
            const Point& center,
            float radius,
            float magnitude,
            float distanceFactor
        ) const;

        void RenderGlowForRing(
            GraphicsContext& context,
            const Point& center,
            float radius,
            float strokeWidth,
            float magnitude,
            const Color& baseColor
        ) const;

        void RenderMainRing(
            GraphicsContext& context,
            const Point& center,
            float radius,
            float strokeWidth,
            const Color& color
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color CalculateRingColor(
            float magnitude,
            float distanceFactor
        ) const;

        [[nodiscard]] float CalculateRingRadius(
            int index,
            float ringStep,
            float magnitude
        ) const;

        [[nodiscard]] float CalculateStrokeWidth(float magnitude) const;

        [[nodiscard]] static float GetRingMagnitude(
            const SpectrumData& spectrum,
            int ringIndex,
            int ringCount
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        float m_angle;
        float m_waveTime;
    };

} // namespace Spectrum

#endif