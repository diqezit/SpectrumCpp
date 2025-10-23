#ifndef SPECTRUM_CPP_CONTROLLER_CORE_H
#define SPECTRUM_CPP_CONTROLLER_CORE_H

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
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

    class ControllerCore final
    {
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

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept;
        [[nodiscard]] AudioManager* GetAudioManager() const noexcept;
        [[nodiscard]] Platform::WindowManager* GetWindowManager() const noexcept;

    private:
        bool InitializeSubsystems();

        void MainLoop();
        void ProcessFrame(const FrameState& frameState);

        [[nodiscard]] FrameState CollectFrameState() const;
        void ProcessInputAndUpdate(const FrameState& frameState);

        void RenderVisualization(const FrameState& frameState);
        void RenderUI(const FrameState& frameState);
        void RenderSettingsButton(const FrameState& frameState);

        [[nodiscard]] bool CanRenderVisualization(const FrameState& frameState) const;
        [[nodiscard]] bool CanRenderUI() const;
        [[nodiscard]] bool ShouldProcessFrame() const;

        void HandleVisualizationDeviceLoss();
        void HandleUIDeviceLoss();

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

        Rect m_settingsButtonRect;
    };

}  // namespace Spectrum

#endif  // SPECTRUM_CPP_CONTROLLER_CORE_H