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

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class Canvas;

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
            Canvas& canvas,
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

        void RenderRing(
            Canvas& canvas,
            const Point& center,
            float radius,
            float magnitude,
            float distanceFactor
        ) const;

        void RenderRingGlow(
            Canvas& canvas,
            const Point& center,
            float radius,
            float strokeWidth,
            float magnitude,
            const Color& baseColor
        ) const;

        void RenderRingShape(
            Canvas& canvas,
            const Point& center,
            float radius,
            float strokeWidth,
            const Color& color
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateRotationAngle(
            float avgIntensity,
            float deltaTime
        );

        void UpdateWavePhase(float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Point GetViewportCenter() const;
        [[nodiscard]] float GetMaxRadius() const;

        [[nodiscard]] float CalculateRingStep(
            float maxRadius,
            int ringCount
        ) const;

        [[nodiscard]] float CalculateRingRadius(
            int index,
            float ringStep,
            float magnitude
        ) const;

        [[nodiscard]] float CalculateDistanceFactor(
            float radius,
            float maxRadius
        ) const;

        [[nodiscard]] float CalculateStrokeWidth(float magnitude) const;

        [[nodiscard]] std::pair<float, float> GetRingRadii(
            float centerRadius,
            float strokeWidth
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color CalculateRingColor(
            float magnitude,
            float distanceFactor
        ) const;

        [[nodiscard]] Color CalculateGlowColor(const Color& baseColor) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Data Extraction
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] int GetEffectiveRingCount(const SpectrumData& spectrum) const;

        [[nodiscard]] static float GetRingMagnitude(
            const SpectrumData& spectrum,
            int ringIndex,
            int ringCount
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsRingVisible(float magnitude) const;

        [[nodiscard]] bool IsRingInBounds(
            float radius,
            float maxRadius
        ) const;

        [[nodiscard]] bool CanRenderRingShape(
            float innerRadius,
            float outerRadius
        ) const;

        [[nodiscard]] bool ShouldRenderGlow(float magnitude) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        QualitySettings m_settings;
        float m_angle;
        float m_waveTime;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H