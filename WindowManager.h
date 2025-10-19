// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Defines the WindowManager, which orchestrates the lifecycle and
// configuration of the main application window and the overlay.
// It manages the graphics context, UI, and state transitions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

#include "Common.h"
#include "MainWindow.h"

namespace Spectrum {

    class ControllerCore;
    class EventBus;
    class GraphicsContext;
    class UIManager;
    class IRenderer;

    class WindowManager {
    public:
        explicit WindowManager(HINSTANCE hInstance, ControllerCore* controller, EventBus* bus);
        ~WindowManager();

        bool Initialize();
        void ProcessMessages();

        void ToggleOverlay();

        bool IsRunning() const;
        bool IsOverlayMode() const { return m_isOverlay; }
        bool IsActive() const;

        GraphicsContext* GetGraphics() const { return m_graphics.get(); }
        UIManager* GetUIManager() const { return m_uiManager.get(); }
        HWND GetCurrentHwnd() const;
        ControllerCore* GetController() const { return m_controller; }
        MainWindow* GetMainWindow() const { return m_mainWnd.get(); }

        bool RecreateGraphicsAndNotify(HWND hwnd);

    private:
        void SubscribeToEvents(EventBus* bus);
        bool InitializeMainWindow();
        bool InitializeOverlayWindow();
        bool InitializeUI();

        void ActivateOverlayMode();
        void DeactivateOverlayMode();

        bool RecreateGraphicsContext(HWND hwnd);
        void NotifySubsystemsOfResize(HWND hwnd);
        void NotifyUIManagerOfResize();
        void NotifyControllerOfResize(HWND hwnd);

        void NotifyRendererOfModeChange();
        IRenderer* GetActiveRenderer();

        void HideMainUIAndWindow();
        void PositionAndShowOverlayWindow();
        void HideOverlayWindow();
        void ShowMainUIAndWindow();

        void OnExitRequest();

        HINSTANCE m_hInstance;
        ControllerCore* m_controller;
        bool m_isOverlay;

        std::unique_ptr<MainWindow> m_mainWnd;
        std::unique_ptr<MainWindow> m_overlayWnd;

        std::unique_ptr<GraphicsContext> m_graphics;
        std::unique_ptr<UIManager> m_uiManager;
    };

}

#endif