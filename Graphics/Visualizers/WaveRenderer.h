#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class Canvas;

    class WaveRenderer final : public BaseRenderer<WaveRenderer>
    {
    public:
        WaveRenderer();
        ~WaveRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::Wave;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Wave";
        }

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(Canvas& canvas, const SpectrumData& spectrum) override;

    private:
        using Settings = Settings::WaveSettings;

        void RenderWaveform(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float width,
            bool reflected
        ) const;

        [[nodiscard]] Color CalculateGlowColor(int layerIndex) const;
        [[nodiscard]] float CalculateGlowWidth(int layerIndex) const;
        [[nodiscard]] int GetGlowLayerCount() const;
        [[nodiscard]] float GetLineWidth() const;

        Settings m_settings;
        float m_smoothedIntensity;
    };

} // namespace Spectrum

#endif