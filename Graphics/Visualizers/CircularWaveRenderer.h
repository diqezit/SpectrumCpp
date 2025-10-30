#ifndef SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H
#define SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class Canvas;

    class CircularWaveRenderer final : public BaseRenderer<CircularWaveRenderer>
    {
    public:
        CircularWaveRenderer();
        ~CircularWaveRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::CircularWave;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Circular Wave";
        }

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(Canvas& canvas, const SpectrumData& spectrum) override;

    private:
        using Settings = Settings::CircularWaveSettings;

        [[nodiscard]] float CalculateRingRadius(
            int index,
            float ringStep,
            float magnitude
        ) const;

        [[nodiscard]] static float GetRingMagnitude(
            const SpectrumData& spectrum,
            int ringIndex,
            int ringCount
        );

        Settings m_settings;
        float m_angle;
        float m_waveTime;
    };

} // namespace Spectrum

#endif