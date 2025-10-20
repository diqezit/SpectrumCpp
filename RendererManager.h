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
#include <string_view>

namespace Spectrum {

    class EventBus;
    class WindowManager;
    class IRenderer;

    class RendererManager {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        explicit RendererManager(
            EventBus* bus,
            WindowManager* windowManager
        );
        ~RendererManager();

        bool Initialize();
        void OnResize(int width, int height);

        void SetCurrentRenderer(
            RenderStyle style,
            GraphicsContext* graphics
        );
        void SwitchToNextRenderer(GraphicsContext* graphics);
        void SwitchToPrevRenderer(GraphicsContext* graphics);
        void CycleQuality(int direction);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] IRenderer* GetCurrentRenderer() const;
        [[nodiscard]] RenderStyle GetCurrentStyle() const;
        [[nodiscard]] RenderQuality GetQuality() const;
        [[nodiscard]] std::string_view GetCurrentRendererName() const;
        [[nodiscard]] std::string_view GetQualityName() const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void SubscribeToEvents(EventBus* bus);
        void CreateRenderers();
        void ActivateInitialRenderer();

        void DeactivateCurrentRenderer();
        void ActivateNewRenderer(
            RenderStyle style,
            GraphicsContext* graphics
        );

        void SetQuality(RenderQuality quality);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_currentRenderer;
        RenderStyle m_currentStyle;
        RenderQuality m_currentQuality;
        WindowManager* m_windowManager;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDERER_MANAGER_H