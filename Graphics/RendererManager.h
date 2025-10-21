#ifndef SPECTRUM_CPP_RENDERER_MANAGER_H
#define SPECTRUM_CPP_RENDERER_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the RendererManager, responsible for managing the lifecycle
// of all available visualizers (IRenderer implementations).
// 
// This class handles the creation, switching, and configuration of
// renderers, acting as a central authority for visualization style and
// quality settings. It depends on EventBus for input and WindowManager
// for viewport information.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <map>
#include <memory>
#include <string_view>

namespace Spectrum
{
    class EventBus;
    class WindowManager;
    class IRenderer;

    class RendererManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit RendererManager(EventBus* bus, WindowManager* windowManager);
        ~RendererManager();

        RendererManager(const RendererManager&) = delete;
        RendererManager& operator=(const RendererManager&) = delete;

        [[nodiscard]] bool Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Event Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void OnResize(int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // The Canvas/GraphicsContext parameter is no longer needed.
        void SetCurrentRenderer(RenderStyle style);
        void SwitchToNextRenderer();
        void SwitchToPrevRenderer();
        void CycleQuality(int direction);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] IRenderer* GetCurrentRenderer() const;
        [[nodiscard]] RenderStyle GetCurrentStyle() const;
        [[nodiscard]] RenderQuality GetQuality() const;
        [[nodiscard]] std::string_view GetCurrentRendererName() const;
        [[nodiscard]] std::string_view GetQualityName() const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SubscribeToEvents(EventBus* bus);
        void CreateRenderers();
        void ActivateInitialRenderer();
        void DeactivateCurrentRenderer();
        void ActivateNewRenderer(RenderStyle style);
        void SetQuality(RenderQuality quality);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_currentRenderer; // Non-owning pointer to an element in m_renderers
        RenderStyle m_currentStyle;
        RenderQuality m_currentQuality;
        WindowManager* m_windowManager; // Non-owning pointer to a global service
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDERER_MANAGER_H