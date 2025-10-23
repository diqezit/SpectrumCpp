#ifndef SPECTRUM_CPP_WAVE_RENDERER_H
#define SPECTRUM_CPP_WAVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the WaveRenderer, an enhanced smooth waveform visualizer.
//
// This renderer displays spectrum data as a continuous waveform with
// advanced visual effects including multi-layer glow, shadows, reflections,
// and dynamic brightness adjustments based on audio intensity.
//
// Key features:
// - Quality-dependent glow layers with intensity modulation
// - Optional shadow rendering with configurable blur
// - Optional reflection with transparency (mirrored waveform)
// - Smooth antialiased line rendering via Canvas
// - Dynamic brightness boost for high-intensity audio
// - Configurable line width and effects based on quality
//
// Design notes:
// - All rendering methods are const (stateless rendering)
// - Delegates drawing to Canvas.DrawWaveform
// - Glow rendered in multiple passes (back to front)
// - Intensity smoothing prevents jarring visual changes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

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

        using Settings = Settings::WaveSettings;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Layers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RenderAllLayers(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Rect& bounds
        ) const;

        void RenderShadowLayer(
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
        [[nodiscard]] Color CalculateShadowColor() const;

        [[nodiscard]] float CalculateGlowAlpha(int layerIndex) const;
        [[nodiscard]] float CalculateGlowWidth(int layerIndex) const;

        [[nodiscard]] float GetReflectionAlpha() const;
        [[nodiscard]] float GetGlowReflectionAlpha() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] int GetGlowLayerCount() const;
        [[nodiscard]] float GetLineWidth() const;
        [[nodiscard]] float GetShadowBlur() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ShouldRenderGlow() const;
        [[nodiscard]] bool ShouldRenderReflection() const;
        [[nodiscard]] bool ShouldRenderShadow() const;
        [[nodiscard]] bool IsSpectrumValid(const SpectrumData& spectrum) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Settings m_settings;
        float m_smoothedIntensity;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_WAVE_RENDERER_H