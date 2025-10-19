#ifndef SPECTRUM_CPP_IRENDERER_H
#define SPECTRUM_CPP_IRENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the IRenderer interface, which is the contract for
// all visualizer implementations. It allows the RendererManager to treat
// all visualizers polymorphically
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "GraphicsContext.h"

namespace Spectrum {

    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        // main entry point for drawing a frame
        virtual void Render(GraphicsContext& context, const SpectrumData& spectrum) = 0;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        // user can change quality to balance performance and visuals
        virtual void SetQuality(RenderQuality quality) = 0;

        // allows user to customize the main color of the visualizer
        virtual void SetPrimaryColor(const Color& color) = 0;

        // overlay mode is for drawing on top of other content
        // renderers should use less intrusive visuals in this mode
        virtual void SetOverlayMode(bool isOverlay) = 0;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Information
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        virtual RenderStyle GetStyle() const = 0;
        virtual std::string_view GetName() const = 0;

        // indicates if the visualizer's color can be changed by the user
        // some visualizers have a fixed color scheme
        virtual bool SupportsPrimaryColor() const { return true; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        // called when the renderer becomes active
        // allows renderer to set up its initial state for the given viewport
        virtual void OnActivate(int width, int height) {}

        // called when the renderer is switched out
        // allows for cleanup of any state
        virtual void OnDeactivate() {}
    };

} // namespace Spectrum

#endif