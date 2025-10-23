// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines ControllerCore, the central orchestrator of the application that
// coordinates all major subsystems (window, audio, rendering, input, UI).
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

#include "Common/Common.h"
#include "Common/Timer.h"
#include <memory>
#include <vector>

namespace Spectrum {
    class AudioManager;
    class EventBus;
    class RendererManager;
    class RenderEngine;
    class Canvas;

    namespace Platform {
        class WindowManager;
        namespace Input {
            class InputManager;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Frame State Structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct MouseState {
        Point position{ 0.0f, 0.0f };
        bool leftButtonDown = false;
        bool rightButtonDown = false;
        bool middleButtonDown = false;
        float wheelDelta = 0.0f;
    };

    struct FrameState {
        MouseState mouse;
        float deltaTime = 0.0f;
        uint64_t frameNumber = 0;
        bool isActive = false;
        bool isOverlayMode = false;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // ControllerCore Class
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class ControllerCore final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit ControllerCore(HINSTANCE hInstance);
        ~ControllerCore() noexcept;

        ControllerCore(const ControllerCore&) = delete;
        ControllerCore& operator=(const ControllerCore&) = delete;
        ControllerCore(ControllerCore&&) = delete;
        ControllerCore& operator=(ControllerCore&&) = delete;

        [[nodiscard]] bool Initialize();
        void Run();
        void Shutdown();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // System Event Callbacks
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void OnResize(int width, int height);
        void OnCloseRequest();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetPrimaryColor(const Color& color);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept;
        [[nodiscard]] AudioManager* GetAudioManager() const noexcept;
        [[nodiscard]] Platform::WindowManager* GetWindowManager() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool InitializeManagers();
        [[nodiscard]] bool InitializeWindowManager();
        [[nodiscard]] bool InitializeAudioManager();
        [[nodiscard]] bool InitializeRendererManager();
        [[nodiscard]] bool InitializeUIManager();

        void MainLoop();
        [[nodiscard]] FrameState CollectFrameState();

        void ProcessInput();
        void Update(const FrameState& frameState);
        void Render(const FrameState& frameState);

        [[nodiscard]] bool CanRender(const FrameState& frameState) const;
        void PrepareFrame(const FrameState& frameState) const;
        void RenderSpectrum() const;
        void RenderUI() const;
        void FinalizeFrame();

        [[nodiscard]] bool ShouldProcessFrame() const;
        void ThrottleFrameRate() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static constexpr float kTargetFrameTime = 1.0f / 60.0f;

        HINSTANCE m_hInstance;

        std::unique_ptr<EventBus> m_eventBus;
        std::unique_ptr<Platform::WindowManager> m_windowManager;
        std::unique_ptr<AudioManager> m_audioManager;
        std::unique_ptr<RendererManager> m_rendererManager;
        std::unique_ptr<Platform::Input::InputManager> m_inputManager;

        Utils::Timer m_timer;
        std::vector<InputAction> m_actions;
        uint64_t m_frameCounter;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CONTROLLER_CORE_H