// Renders the spectrum as vertical bars

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
        // map quality preference to concrete render settings
        void UpdateSettings() override;

        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        // settings that change with quality level
        struct Settings {
            float barSpacing;
            float cornerRadius;
            bool useShadow;
            bool useHighlight;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Single Bar Rendering Steps (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // orchestrates drawing of a single bar
        void RenderBar(
            GraphicsContext& context,
            const Rect& rect,
            float magnitude
        );

        // applies effects like shadows
        void RenderBarWithEffects(
            GraphicsContext& context,
            const Rect& rect,
            const Color& color
        );

        // draws the main shape of the bar
        void RenderMainBar(
            GraphicsContext& context,
            const Rect& rect,
            const Color& color
        );

        // draws a highlight on top of the bar
        void RenderHighlight(
            GraphicsContext& context,
            const Rect& rect,
            float magnitude
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // computes final color based on magnitude
        Color CalculateBarColor(float magnitude) const;

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_BARS_RENDERER_H