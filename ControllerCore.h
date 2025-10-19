#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines ControllerCore, the central class that orchestrates
// all major components of the application, including the window, audio,
// rendering, and input managers. It drives the main application loop.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "PlatformUtils.h"
#include "Timer.h"
#include "EventBus.h"
#include <memory>
#include <vector>

namespace Spectrum {

    class WindowManager;
    class AudioManager;
    class RendererManager;
    class InputManager;
    class GraphicsContext;

    class ControllerCore {
    public:
        explicit ControllerCore(HINSTANCE hInstance);
        ~ControllerCore();

        bool Initialize();
        void Run();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // System & Event Callbacks
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        LRESULT HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void OnResize(int width, int height);
        void SetPrimaryColor(const Color& color);
        void OnClose();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Getters for Component Communication
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        RendererManager* GetRendererManager() const { return m_rendererManager.get(); }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Internal Logic
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        bool InitializeManagers();
        bool InitializeEventBus();
        bool InitializeWindowManager();
        bool InitializeInputManager();
        bool InitializeAudioManager();
        bool InitializeRendererManager();
        void PrintWelcomeMessage();
        void MainLoop();
        void ProcessInput();
        void Update(float deltaTime);
        void Render();
        bool CanRender() const;
        void PrepareFrame(GraphicsContext& graphics);
        void RenderSpectrum(GraphicsContext& graphics);
        void RenderUI(GraphicsContext& graphics);
        void FinalizeFrame(GraphicsContext& graphics);
        LRESULT HandleMouseMessage(UINT msg, LPARAM lParam);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member State
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

#endif