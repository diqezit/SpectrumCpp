// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// ControllerCore.h: The main controller class orchestrating all components.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

#include "Common.h"
#include "WindowManager.h"
#include "AudioManager.h"
#include "RendererManager.h"
#include "Utils.h"

namespace Spectrum {

    class InputManager;

    class ControllerCore {
        friend class InputManager;
    public:
        explicit ControllerCore(HINSTANCE hInstance);
        ~ControllerCore();

        bool Initialize();
        void Run();

        // Public method for overlay toggle
        void ToggleOverlay();

    private:
        bool InitializeComponents();
        void PrintWelcomeMessage();

        void MainLoop();
        void Update(float deltaTime);
        void Render();

        void OnResize(int width, int height);
        void OnClose();

    private:
        HINSTANCE m_hInstance;

        // Window management
        std::unique_ptr<WindowManager> m_windowManager;

        // Input
        std::unique_ptr<InputManager> m_inputManager;

        // Audio
        std::unique_ptr<AudioManager> m_audioManager;

        // Renderer
        std::unique_ptr<RendererManager> m_rendererManager;

        // State
        ApplicationState m_state;

        // Timing
        Utils::Timer m_timer;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CONTROLLER_CORE_H