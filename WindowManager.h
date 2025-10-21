// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the WindowManager, which orchestrates the lifecycle and
// configuration of the main application window and the overlay window.
//
// The WindowManager serves as the primary interface to the OS windowing
// system, managing window creation, state transitions (normal/overlay mode),
// ownership of the GraphicsContext, and routing of raw Win32 messages to
// appropriate subsystems. It maintains the current mouse state and provides
// it to the ControllerCore for frame state collection.
//
// Key Responsibilities:
// - Create and manage main and overlay windows
// - Own and recreate GraphicsContext on device loss
// - Route Win32 messages to appropriate handlers
// - Maintain mouse input state
// - Coordinate overlay mode transitions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

#include "Common.h"
#include <memory>

namespace Spectrum {

    class ControllerCore;
    class EventBus;
    class GraphicsContext;
    class MainWindow;
    class UIManager;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // WindowManager Class
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class WindowManager final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct MouseState {
            Point position{ 0.0f, 0.0f };
            bool leftButtonDown = false;
            bool rightButtonDown = false;
            bool middleButtonDown = false;
            float wheelDelta = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit WindowManager(
            HINSTANCE hInstance,
            ControllerCore* controller,
            EventBus* bus
        );
        ~WindowManager() noexcept;

        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;
        WindowManager(WindowManager&&) = delete;
        WindowManager& operator=(WindowManager&&) = delete;

        [[nodiscard]] bool Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ProcessMessages();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Window Message Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        LRESULT HandleWindowMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ToggleOverlay();
        bool RecreateGraphicsAndNotify(HWND hwnd);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsRunning() const;
        [[nodiscard]] bool IsOverlayMode() const noexcept;
        [[nodiscard]] bool IsActive() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] GraphicsContext* GetGraphics() const noexcept;
        [[nodiscard]] UIManager* GetUIManager() const noexcept;
        [[nodiscard]] MainWindow* GetMainWindow() const noexcept;
        [[nodiscard]] HWND GetCurrentHwnd() const;
        [[nodiscard]] const MouseState& GetMouseState() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SubscribeToEvents(EventBus* bus);

        [[nodiscard]] bool InitializeMainWindow();
        [[nodiscard]] bool InitializeOverlayWindow();

        void ActivateOverlayMode();
        void DeactivateOverlayMode();

        void HideMainWindow() const;
        void ShowMainWindow() const;
        void HideOverlayWindow() const;
        void PositionAndShowOverlay() const;

        [[nodiscard]] bool RecreateGraphicsContext(HWND hwnd);
        void PropagateResizeToSubsystems(HWND hwnd);
        void NotifyRendererOfModeChange() const;

        void OnExitRequest();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        HINSTANCE m_hInstance;
        ControllerCore* m_controller;

        bool m_isOverlay;
        MouseState m_mouseState;

        std::unique_ptr<MainWindow> m_mainWnd;
        std::unique_ptr<MainWindow> m_overlayWnd;
        std::unique_ptr<GraphicsContext> m_graphics;
        std::unique_ptr<UIManager> m_uiManager;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_MANAGER_H