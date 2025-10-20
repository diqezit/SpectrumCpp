// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the WindowManager, which orchestrates the lifecycle and
// configuration of the main application window and the overlay.
// It manages the graphics context, UI, and state transitions.
//
// Defines the WindowManager, the primary interface to the OS's windowing
// system. It orchestrates the creation of all windows, manages their state
// (normal/overlay), owns the GraphicsContext, and routes raw window messages
// to the appropriate subsystems.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

#include "Common.h"
#include <memory>

namespace Spectrum {

    class ControllerCore;
    class EventBus;
    class GraphicsContext;
    class IRenderer;
    class MainWindow;
    class UIManager;

    class WindowManager final {
    public:
        struct MouseState {
            Point position{ 0, 0 };
            bool leftButtonDown = false;
        };

        explicit WindowManager(
            HINSTANCE hInstance,
            ControllerCore* controller,
            EventBus* bus
        );
        ~WindowManager() noexcept;

        [[nodiscard]] bool Initialize();
        bool RecreateGraphicsAndNotify(HWND hwnd);

        void ProcessMessages();

        LRESULT HandleWindowMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

        void ToggleOverlay();
        [[nodiscard]] bool IsRunning() const;
        [[nodiscard]] bool IsOverlayMode() const noexcept;
        [[nodiscard]] bool IsActive() const;

        [[nodiscard]] GraphicsContext* GetGraphics() const noexcept;
        [[nodiscard]] UIManager* GetUIManager() const noexcept;
        [[nodiscard]] HWND GetCurrentHwnd() const;
        [[nodiscard]] MainWindow* GetMainWindow() const noexcept;
        [[nodiscard]] const MouseState& GetMouseState() const noexcept { return m_mouseState; }

    private:
        void SubscribeToEvents(EventBus* bus);

        [[nodiscard]] bool InitializeMainWindow();
        [[nodiscard]] bool InitializeOverlayWindow();

        void ActivateOverlayMode();
        void DeactivateOverlayMode();

        void HideMainWindow() const;
        void PositionAndShowOverlay() const;
        void HideOverlayWindow() const;
        void ShowMainWindow() const;

        [[nodiscard]] bool RecreateGraphicsContext(HWND hwnd);
        void PropagateResizeToSubsystems(HWND hwnd);

        void NotifyRendererOfModeChange() const;
        void OnExitRequest();

        HINSTANCE m_hInstance;
        ControllerCore* m_controller;
        bool m_isOverlay;
        MouseState m_mouseState;

        std::unique_ptr<MainWindow> m_mainWnd;
        std::unique_ptr<MainWindow> m_overlayWnd;
        std::unique_ptr<GraphicsContext> m_graphics;
        std::unique_ptr<UIManager> m_uiManager;
    };

}

#endif