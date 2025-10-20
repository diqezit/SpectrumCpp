// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines ControllerCore, the central class that orchestrates
// all major components of the application, including the window, audio,
// rendering, and input managers. It drives the main application loop.
//
// Defines the ControllerCore, the application's central orchestrator. It is
// responsible for initializing all major subsystems and driving the main
// high-level update/render loop, completely decoupled from platform-specific
// API details.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

#include "Common.h"
#include "EventBus.h"
#include "Timer.h"
#include <memory>
#include <vector>

namespace Spectrum {

    // Forward declarations
    class AudioManager;
    class GraphicsContext;
    class InputManager;
    class RendererManager;
    class WindowManager;

    class ControllerCore final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        explicit ControllerCore(HINSTANCE hInstance);
        ~ControllerCore() noexcept;

        [[nodiscard]] bool Initialize();
        void Run();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // System Event Callbacks
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void OnResize(int width, int height);
        void OnCloseRequest();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void SetPrimaryColor(const Color& color);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept;
        [[nodiscard]] AudioManager* GetAudioManager() const noexcept;
        [[nodiscard]] WindowManager* GetWindowManager() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] bool InitializeManagers();

        void MainLoop();
        void ProcessInput();
        void Update(float deltaTime);
        void Render();

        [[nodiscard]] bool CanRender() const;
        void PrepareFrame(GraphicsContext& graphics) const;
        void RenderSpectrum(GraphicsContext& graphics) const;
        void RenderUI(GraphicsContext& graphics) const;
        void FinalizeFrame(GraphicsContext& graphics) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        HINSTANCE m_hInstance;

        std::unique_ptr<WindowManager> m_windowManager;
        std::unique_ptr<AudioManager> m_audioManager;
        std::unique_ptr<RendererManager> m_rendererManager;
        std::unique_ptr<InputManager> m_inputManager;
        std::unique_ptr<EventBus> m_eventBus;

        Utils::Timer m_timer;
        std::vector<InputAction> m_actions;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CONTROLLER_CORE_H