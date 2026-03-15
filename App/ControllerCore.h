#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Application controller — owns all subsystems, drives the main loop.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Platform/MessageHandlerBase.h"
#include <memory>

namespace Spectrum {

    class AudioManager;
    class EventBus;
    class RendererManager;
    class RenderEngine;

    namespace Platform {
        class WindowManager;
        class InputManager;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Per-frame snapshot
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    struct FrameState {
        Platform::MouseState mouse;
        float    deltaTime = 0.0f;
        uint64_t frameNumber = 0;
        bool     isActive = false;
        bool     isOverlay = false;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Core controller
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    class ControllerCore final {
    public:
        explicit ControllerCore(HINSTANCE hInstance);
        ~ControllerCore() noexcept;

        ControllerCore(const ControllerCore&) = delete;
        ControllerCore& operator=(const ControllerCore&) = delete;

        [[nodiscard]] bool Initialize();
        void Run();
        void Shutdown();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Event callbacks
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void OnResize(int w, int h);
        void OnUIResize(int w, int h);
        void OnCloseRequest();
        void OnMainWindowClick(const Point& pos);
        void SetPrimaryColor(const Color& color);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Accessors
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept { return m_rendererMgr.get(); }
        [[nodiscard]] AudioManager* GetAudioManager()   const noexcept { return m_audioMgr.get(); }
        [[nodiscard]] Platform::WindowManager* GetWindowManager()  const noexcept { return m_windowMgr.get(); }

    private:
        bool InitializeSubsystems();
        void MainLoop();
        void ProcessFrame();

        [[nodiscard]] FrameState CollectFrameState() const;
        void ProcessInput(float dt);
        void RenderVisualization(const FrameState& fs);
        void RenderUI();
        void RenderSettingsButton(const FrameState& fs);
        void HandleDeviceLoss(RenderEngine* engine);

        [[nodiscard]] bool ShouldProcessFrame() const {
            return m_timer.GetElapsedSeconds() >= kFrameTime;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        static constexpr float kFps = 60.0f;
        static constexpr float kFrameTime = 1.0f / kFps;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        HINSTANCE m_hInstance;

        std::unique_ptr<EventBus>                   m_eventBus;
        std::unique_ptr<Platform::WindowManager>    m_windowMgr;
        std::unique_ptr<AudioManager>               m_audioMgr;
        std::unique_ptr<RendererManager>             m_rendererMgr;
        std::unique_ptr<Platform::InputManager>      m_inputMgr;

        Helpers::Utils::Timer m_timer;
        uint64_t m_frameCounter = 0;
        Rect     m_settingsBtnRect;
    };

} // namespace Spectrum

#endif