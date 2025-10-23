#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the main application orchestrator
//
// Responsibilities:
// - Owns and manages the lifecycle of all major application subsystems
// - Uses an immutable FrameState to prevent mid-frame state changes
// - Implements a fixed-timestep loop to decouple simulation from render speed
// - Separates frame processing into Input Update Render phases for clarity
// - Orchestrates automatic recovery from graphics device loss
//
// Design notes:
// - Uses non-owning raw pointers for inter-system communication to avoid circular dependencies
// - Propagates all initialization failures to the application entry point for a clean shutdown on error
// - Ensures a graceful and ordered shutdown of all subsystems in the destructor
// - Collects a single immutable state snapshot at the start of each frame
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Helpers/Utils/Timer.h"
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

    class ControllerCore final
    {
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
        // Configuration
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
        // Subsystem Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        bool InitializeSubsystems();
        bool InitializeEventBus();
        bool InitializeWindowManager();
        bool InitializeInputManager();
        bool InitializeAudioManager();
        bool InitializeRendererManager();
        bool InitializeUIManager();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void MainLoop();
        void ProcessFrame(const FrameState& frameState);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Frame State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] FrameState CollectFrameState() const;
        [[nodiscard]] MouseState CollectMouseState() const;
        [[nodiscard]] float CalculateDeltaTime() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Input Processing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ProcessInput();
        void UpdateInputManager();
        void PublishInputActions();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Update Phase
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const FrameState& frameState);
        void UpdateAudioSystem(float deltaTime);
        void UpdateUISystem(const FrameState& frameState);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Render Phase
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Render(const FrameState& frameState);
        [[nodiscard]] bool PrepareFrame(const FrameState& frameState);
        void RenderContent();
        void RenderSpectrum();
        void RenderUI();
        void FinalizeFrame();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Render Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CanRender(const FrameState& frameState) const;
        [[nodiscard]] bool IsWindowOccluded() const;
        [[nodiscard]] bool ShouldRenderInOverlayMode() const;
        [[nodiscard]] bool ShouldRenderInNormalMode(const FrameState& frameState) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Frame Timing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ShouldProcessFrame() const;
        void ThrottleFrameRate() const;
        void ResetFrameTimer();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Graphics Recovery
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void HandleDeviceLoss();
        bool RecreateGraphicsContext();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ValidateManagers() const noexcept;
        [[nodiscard]] bool ValidateWindowManager() const noexcept;
        [[nodiscard]] bool ValidateAudioManager() const noexcept;
        [[nodiscard]] bool ValidateRendererManager() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Utility Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] RenderEngine* GetRenderEngine() const noexcept;
        [[nodiscard]] Color GetClearColor(bool isOverlayMode) const noexcept;

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

        Helpers::Utils::Timer m_timer;
        std::vector<InputAction> m_actions;
        uint64_t m_frameCounter;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CONTROLLER_CORE_H