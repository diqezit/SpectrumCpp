#ifndef SPECTRUM_CPP_IRENDERER_H
#define SPECTRUM_CPP_IRENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the IRenderer interface, the contract for all visualizer
// implementations.
// 
// This interface allows the RendererManager to manage and interact with all
// visualizers polymorphically, abstracting away their concrete
// implementations. It follows the Interface Segregation Principle by
// providing default implementations for non-essential lifecycle methods.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <string_view>

namespace Spectrum
{
    // Forward declaration
    class Canvas;

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Main entry point for drawing a single frame.
        virtual void Render(Canvas& canvas, const SpectrumData& spectrum) = 0;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Sets the quality to balance performance and visuals.
        virtual void SetQuality(RenderQuality quality) = 0;

        // Sets the main color of the visualizer.
        virtual void SetPrimaryColor(const Color& /*color*/) {}

        // Sets overlay mode for drawing on top of other content.
        virtual void SetOverlayMode(bool /*isOverlay*/) {}

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries & Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] virtual RenderStyle GetStyle() const = 0;
        [[nodiscard]] virtual std::string_view GetName() const = 0;

        // Indicates if the visualizer's color can be changed by the user.
        [[nodiscard]] virtual bool SupportsPrimaryColor() const { return true; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Called when the renderer becomes active or viewport changes.
        virtual void OnActivate(int /*width*/, int /*height*/) {}

        // Called when the renderer is switched out. Allows for state cleanup.
        virtual void OnDeactivate() {}
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_IRENDERER_H