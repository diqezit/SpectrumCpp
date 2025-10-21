// WaveRenderer.h
#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the WaveRenderer, a smooth waveform visualizer.
//
// This renderer displays spectrum data as a continuous waveform with
// optional glow effects and reflection. The waveform represents the
// frequency spectrum as a flowing curve.
//
// Key features:
// - Quality-dependent glow layers (multi-pass for depth)
// - Optional reflection with transparency (mirrored waveform)
// - Smooth line rendering via GraphicsContext
// - Configurable line width based on quality
//
// Design notes:
// - All rendering methods are const (stateless rendering)
// - Delegates drawing to GraphicsContext.DrawWaveform
// - Glow rendered in multiple passes (back to front)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BaseRenderer.h"

namespace Spectrum {

    class WaveRenderer final : public BaseRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        WaveRenderer();
        ~WaveRenderer() override = default;

        WaveRenderer(const WaveRenderer&) = delete;
        WaveRenderer& operator=(const WaveRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderStyle GetStyle() const override { return RenderStyle::Wave; }
        [[nodiscard]] std::string_view GetName() const override { return "Wave"; }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BaseRenderer Overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSettings() override;

        void DoRender(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Settings
        {
            float lineWidth;
            bool useGlow;
            bool useReflection;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
    };

} // namespace Spectrum

#endif