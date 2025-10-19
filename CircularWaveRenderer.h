// Renders spectrum as animated concentric rings

#ifndef SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H
#define SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class CircularWaveRenderer final : public BaseRenderer {
    public:
        CircularWaveRenderer();
        ~CircularWaveRenderer() override = default;

        RenderStyle GetStyle() const override {
            return RenderStyle::CircularWave;
        }

        std::string_view GetName() const override {
            return "Circular Wave";
        }

    protected:
        // map quality preference to concrete render settings
        void UpdateSettings() override;

        // advance animation timers based on spectrum intensity
        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    private:
        // settings that change with quality level
        struct QualitySettings {
            bool useGlow;
            float maxStroke;
            int maxRings;
            float rotationSpeed;
            float waveSpeed;
        };

        // --- SRP Refactored Render Steps ---
        void RenderCalculatedRing(
            GraphicsContext& context,
            const Point& center,
            float radius,
            float magnitude,
            float distanceFactor
        );

        void RenderGlowForRing(
            GraphicsContext& context,
            const Point& center,
            float radius,
            float strokeWidth,
            float magnitude,
            const Color& baseColor
        );

        void RenderMainRing(
            GraphicsContext& context,
            const Point& center,
            float radius,
            float strokeWidth,
            const Color& color
        );

        // --- Calculation Helpers ---
        Color CalculateRingColor(float magnitude, float distanceFactor) const;

        float CalculateRingRadius(
            int index,
            float ringStep,
            float magnitude
        ) const;

        float CalculateStrokeWidth(float magnitude) const;


        static float GetRingMagnitude(
            const SpectrumData& spectrum,
            int ringIndex,
            int ringCount
        );

        QualitySettings m_settings;
        float m_angle;
        float m_waveTime;
    };
}

#endif // SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H