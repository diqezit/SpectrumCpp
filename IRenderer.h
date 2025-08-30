// IRenderer.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// IRenderer.h: Interface for all visualizer rendering classes.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_IRENDERER_H
#define SPECTRUM_CPP_IRENDERER_H

#include "Common.h"
#include "GraphicsContext.h"

namespace Spectrum {

    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        // Main rendering function
        virtual void Render(GraphicsContext& context, const SpectrumData& spectrum) = 0;

        // Configuration methods
        virtual void SetQuality(RenderQuality quality) = 0;
        virtual void SetPrimaryColor(const Color& color) = 0;
        virtual void SetBackgroundColor(const Color& color) = 0;

        // Informational methods
        virtual RenderStyle GetStyle() const = 0;
        virtual std::string_view GetName() const = 0;
        virtual bool SupportsQualityLevels() const { return true; }
        virtual bool SupportsPrimaryColor() const { return true; }

        // Called when the renderer is activated
        virtual void OnActivate(int width, int height) {}

        // Called when the renderer is deactivated
        virtual void OnDeactivate() {}
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_IRENDERER_H