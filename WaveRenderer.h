// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// WaveRenderer.h: Renders the spectrum as a continuous waveform.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class WaveRenderer final : public BaseRenderer {
    public:
        WaveRenderer();
        ~WaveRenderer() override = default;

        RenderStyle GetStyle() const override { return RenderStyle::Wave; }
        std::string_view GetName() const override { return "Wave"; }

    protected:
        void UpdateSettings() override;
        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        struct Settings {
            float lineWidth;
            bool useReflection;
            float reflectionStrength;
        };

        Settings m_settings;
        std::vector<Point> m_points;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_WAVE_RENDERER_H