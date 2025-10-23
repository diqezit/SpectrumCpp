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
// - Smooth line rendering via Canvas
// - Configurable line width based on quality
//
// Design notes:
// - All rendering methods are const (stateless rendering)
// - Delegates drawing to Canvas.DrawWaveform
// - Glow rendered in multiple passes (back to front)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"

namespace Spectrum {

    class Canvas;

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
            Canvas& canvas,
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

        void RenderAllLayers(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds
        ) const;

        void RenderGlowEffect(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds
        ) const;

        void RenderGlowLayer(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds,
            int layerIndex
        ) const;

        void RenderMainWaveform(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds
        ) const;

        void RenderWaveform(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float width,
            bool reflected
        ) const;

        void RenderReflection(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float width
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Rect GetRenderBounds() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color CalculateGlowColor(int layerIndex) const;
        [[nodiscard]] Color CalculateReflectionColor(const Color& baseColor) const;

        [[nodiscard]] float CalculateGlowAlpha(int layerIndex) const;
        [[nodiscard]] float CalculateGlowWidth(int layerIndex) const;

        [[nodiscard]] float GetReflectionAlpha() const;
        [[nodiscard]] float GetGlowReflectionAlpha() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] int GetGlowLayerCount() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ShouldRenderGlow() const;
        [[nodiscard]] bool ShouldRenderReflection() const;
        [[nodiscard]] bool IsSpectrumValid(const SpectrumData& spectrum) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_WAVE_RENDERER_H