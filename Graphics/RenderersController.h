#ifndef SPECTRUM_CPP_RENDERERS_CONTROLLER_H
#define SPECTRUM_CPP_RENDERERS_CONTROLLER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// RenderersController.h: Manages all rendering components and states.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Common/Common.h"
#include "Graphics/IRenderer.h"
#include "Common/SpectrumTypes.h"
#include <map>
#include <memory>

namespace Spectrum {

    class RenderersController {
    public:
        RenderersController();
        ~RenderersController();

        // Initialization and cleanup
        bool InitializeRenderers();
        void Shutdown();

        // Rendering operations
        void RenderCurrentVisualizer(
            const SpectrumData& spectrum
        );

        // Renderer management
        void SetCurrentRenderer(RenderStyle style);
        void SwitchRenderer(int direction = 1);
        RenderStyle GetCurrentRendererStyle() const;
        IRenderer* GetCurrentRenderer() const;

        // Window management
        void OnWindowResize(int width, int height);

        // Color management
        void SetPrimaryColor(const Color& color);

    private:
        void InitializeRendererInstances();

    private:
        // Renderers storage
        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_currentRenderer;
        RenderStyle m_currentStyle;

        // State tracking
        bool m_initialized;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDERERS_CONTROLLER_H