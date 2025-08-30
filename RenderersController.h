// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// RenderersController.h: Manages all rendering components and states.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_RENDERERS_CONTROLLER_H
#define SPECTRUM_CPP_RENDERERS_CONTROLLER_H

#include "Common.h"
#include "IRenderer.h"
#include "GraphicsContext.h"
#include "ColorPicker.h"
#include <map>
#include <memory>

namespace Spectrum {

    class RenderersController {
    public:
        RenderersController();
        ~RenderersController();

        // Initialization and cleanup
        bool InitializeRenderers();
        bool InitializeColorPicker(GraphicsContext& graphics);
        void Shutdown();

        // Rendering operations
        void RenderCurrentVisualizer(
            GraphicsContext& graphics,
            const SpectrumData& spectrum
        );
        void RenderColorPicker(GraphicsContext& graphics);

        // Renderer management
        void SetCurrentRenderer(RenderStyle style);
        void SwitchRenderer(int direction = 1);
        RenderStyle GetCurrentRendererStyle() const;
        IRenderer* GetCurrentRenderer() const;

        // Window management
        void OnWindowResize(int width, int height);

        // Color management
        void SetPrimaryColor(const Color& color);
        ColorPicker* GetColorPicker() const;

        // UI state
        bool IsColorPickerVisible() const;
        void SetColorPickerVisible(bool visible);

    private:
        void InitializeRendererInstances();
        void SetupColorPickerCallback();

    private:
        // Renderers storage
        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_currentRenderer;
        RenderStyle m_currentStyle;

        // UI components
        std::unique_ptr<ColorPicker> m_colorPicker;

        // State tracking
        bool m_initialized;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDERERS_CONTROLLER_H