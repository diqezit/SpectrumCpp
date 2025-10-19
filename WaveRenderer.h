#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class WaveRenderer final : public BaseRenderer {
    public:
        WaveRenderer();
        ~WaveRenderer() override = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        RenderStyle GetStyle() const override {
            return RenderStyle::Wave;
        }

        std::string_view GetName() const override {
            return "Wave";
        }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSettings() override;

        void DoRender(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawGlowEffect(
            GraphicsContext& context,
            const SpectrumData& spectrum,
            const Rect& bounds
        ) const;

        void DrawGlowLayer(
            GraphicsContext& context,
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& glowColor,
            float glowWidth
        ) const;

        void DrawMainWaveform(
            GraphicsContext& context,
            const SpectrumData& spectrum,
            const Rect& bounds
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Settings {
            float lineWidth;
            bool useGlow;
            bool useReflection;
        };

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_WAVE_RENDERER_H