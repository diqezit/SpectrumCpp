#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

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
        namespace Input {
            class InputManager;
        }
    }

    struct FrameState {
        Platform::MouseState mouse;
        float deltaTime = 0.0f;
        uint64_t frameNumber = 0;
        bool isActive = false;
        bool isOverlayMode = false;
    };

    class ControllerCore final {
    public:
        explicit ControllerCore(HINSTANCE hInstance);
        ~ControllerCore() noexcept;

        ControllerCore(const ControllerCore&) = delete;
        ControllerCore& operator=(const ControllerCore&) = delete;
        ControllerCore(ControllerCore&&) = delete;
        ControllerCore& operator=(ControllerCore&&) = delete;

        [[nodiscard]] bool Initialize();
        void Run();
        void Shutdown();

        void OnResize(int width, int height);
        void OnUIResize(int width, int height);
        void OnCloseRequest();
        void OnMainWindowClick(const Point& mousePos);

        void SetPrimaryColor(const Color& color);

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept { return m_rendererManager.get(); }
        [[nodiscard]] AudioManager* GetAudioManager() const noexcept { return m_audioManager.get(); }
        [[nodiscard]] Platform::WindowManager* GetWindowManager() const noexcept { return m_windowManager.get(); }

    private:
        bool InitializeSubsystems();
        void MainLoop();
        void ProcessFrame();

        [[nodiscard]] FrameState CollectFrameState() const;
        void ProcessInputAndUpdate(float deltaTime);
        void RenderVisualization(const FrameState& frameState);
        void RenderUI();
        void RenderSettingsButton(const FrameState& frameState);

        [[nodiscard]] bool ShouldProcessFrame() const;
        void HandleDeviceLoss(RenderEngine* engine);

        static constexpr float FPS_TARGET = 60.0f;
        static constexpr float FRAME_TIME = 1.0f / FPS_TARGET;

        HINSTANCE m_hInstance;
        std::unique_ptr<EventBus> m_eventBus;
        std::unique_ptr<Platform::WindowManager> m_windowManager;
        std::unique_ptr<AudioManager> m_audioManager;
        std::unique_ptr<RendererManager> m_rendererManager;
        std::unique_ptr<Platform::Input::InputManager> m_inputManager;

        Helpers::Utils::Timer m_timer;
        uint64_t m_frameCounter = 0;
        Rect m_settingsButtonRect;
    };

}

#endif