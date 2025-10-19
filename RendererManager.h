// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the RendererManager, which is responsible for managing
// all available visualizers, handling switching between them, and
// orchestrating the main scene rendering process.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_RENDERER_MANAGER_H
#define SPECTRUM_CPP_RENDERER_MANAGER_H

#include "Common.h"
#include "GraphicsContext.h"
#include <map>
#include <memory>

namespace Spectrum {

    class EventBus;
    class WindowManager;
    class IRenderer;
    class ColorPicker;

    class RendererManager {
    public:
        explicit RendererManager(EventBus* bus, WindowManager* windowManager);
        ~RendererManager();

        bool Initialize();

        void RenderScene(
            GraphicsContext& graphics,
            const SpectrumData& spectrum,
            ColorPicker* colorPicker,
            bool isOverlay
        );

        void OnResize(int width, int height);

        void SetCurrentRenderer(RenderStyle style, GraphicsContext* graphics);
        void SwitchToNextRenderer(GraphicsContext* graphics);
        void CycleQuality();

        IRenderer* GetCurrentRenderer() const { return m_currentRenderer; }
        RenderStyle GetCurrentStyle() const { return m_currentStyle; }
        RenderQuality GetQuality() const { return m_currentQuality; }

    private:
        void SubscribeToEvents(EventBus* bus);
        void CreateRenderers();
        void ActivateInitialRenderer();

        void DeactivateCurrentRenderer();
        void ActivateNewRenderer(RenderStyle style, GraphicsContext* graphics);

        bool ShouldSkipRendering(GraphicsContext& graphics) const;
        Color GetClearColor(bool isOverlay) const;
        void RenderVisualizer(GraphicsContext& graphics, const SpectrumData& spectrum);
        void RenderUI(GraphicsContext& graphics, ColorPicker* colorPicker, bool isOverlay);

        void SetQuality(RenderQuality quality);
        const char* GetQualityName(RenderQuality quality) const;

        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_currentRenderer = nullptr;
        RenderStyle m_currentStyle = RenderStyle::Bars;
        RenderQuality m_currentQuality = RenderQuality::Medium;
        WindowManager* m_windowManager = nullptr;
    };

} // namespace Spectrum

#endif