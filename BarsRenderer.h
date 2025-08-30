// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BarsRenderer.h: Renders the spectrum as vertical bars.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_BARS_RENDERER_H
#define SPECTRUM_CPP_BARS_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class BarsRenderer final : public BaseRenderer {
    public:
        BarsRenderer();
        ~BarsRenderer() override = default;

        RenderStyle GetStyle() const override { return RenderStyle::Bars; }
        std::string_view GetName() const override { return "Bars"; }

    protected:
        void UpdateSettings() override;
        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        void RenderBar(GraphicsContext& context,
            const Rect& rect,
            float magnitude);

        struct Settings {
            float barSpacing;
            float cornerRadius;
            bool useShadow;
            bool useHighlight;
        };

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_BARS_RENDERER_H