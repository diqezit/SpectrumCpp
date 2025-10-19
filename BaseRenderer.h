#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the BaseRenderer class, an abstract base for all
// visualizers. It implements the IRenderer interface and provides common
// functionality like quality settings, timing, and viewport management
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "IRenderer.h"
#include "Common.h"

namespace Spectrum {

    class BaseRenderer : public IRenderer {
    public:
        BaseRenderer();
        ~BaseRenderer() override = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void SetQuality(RenderQuality quality) override;
        void SetPrimaryColor(const Color& color) override;
        void SetOverlayMode(bool isOverlay) override;
        void OnActivate(int width, int height) override;
        void Render(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) override;

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Virtual Methods for Child Classes
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        virtual void UpdateSettings() {}

        virtual void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) {
        }

        virtual void DoRender(
            GraphicsContext& context,
            const SpectrumData& spectrum
        ) = 0;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Helper Methods for Child Classes
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        bool IsRenderable(const SpectrumData& spectrum) const noexcept;

        Rect CalculatePaddedRect() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        RenderQuality m_quality;
        Color m_primaryColor;
        bool m_isOverlay;
        int m_width;
        int m_height;
        float m_time;

        // Configuration for aspect-ratio aware renderers
        float m_aspectRatio;
        float m_padding;

    private:
        void UpdateTime(float deltaTime);
        void SetViewport(int width, int height) noexcept;

        static constexpr float TIME_RESET_THRESHOLD = 1e6f;
    };

} // namespace Spectrum

#endif