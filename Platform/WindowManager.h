// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the WindowManager, which orchestrates the lifecycle and
// configuration of the application's windows.
//
// The WindowManager serves as the primary high-level interface to the
// windowing system, managing window creation, state transitions (normal vs.
// overlay), and ownership of the RenderEngine. It delegates raw message
// processing to a dedicated MessageHandler.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

#include "Common/Common.h"
#include <memory>

namespace Spectrum {
    class ControllerCore;
    class EventBus;
    class RenderEngine;
    class UIManager;

    namespace Platform {
        class MainWindow;
        class MessageHandler;

        class WindowManager final {
        public:
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Lifecycle Management
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            explicit WindowManager(HINSTANCE hInstance, ControllerCore* controller, EventBus* bus);
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
            void PropagateResizeToSubsystems(HWND hwnd);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // State Management
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void ToggleOverlay();
            [[nodiscard]] bool RecreateGraphicsAndNotify(HWND hwnd);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // State Queries
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] bool IsRunning() const;
            [[nodiscard]] bool IsOverlayMode() const noexcept;
            [[nodiscard]] bool IsActive() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Public Getters
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] RenderEngine* GetRenderEngine() const noexcept;
            [[nodiscard]] UIManager* GetUIManager() const noexcept;
            [[nodiscard]] MessageHandler* GetMessageHandler() const noexcept;
            [[nodiscard]] MainWindow* GetMainWindow() const noexcept;
            [[nodiscard]] HWND GetCurrentHwnd() const;

        private:
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Private Implementation / Internal Helpers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] bool InitializeMainWindow();
            [[nodiscard]] bool InitializeOverlayWindow();

            void ActivateOverlayMode();
            void DeactivateOverlayMode();

            void HideMainWindow() const;
            void ShowMainWindow() const;
            void HideOverlayWindow() const;
            void PositionAndShowOverlay() const;

            [[nodiscard]] bool RecreateGraphicsContext(HWND hwnd);
            void NotifyRendererOfModeChange() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Member Variables
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            HINSTANCE m_hInstance;
            ControllerCore* m_controller; // Non-owning pointer to parent

            bool m_isOverlay;

            std::unique_ptr<MainWindow> m_mainWnd;
            std::unique_ptr<MainWindow> m_overlayWnd;
            std::unique_ptr<RenderEngine> m_engine;
            std::unique_ptr<UIManager> m_uiManager;
            std::unique_ptr<MessageHandler> m_messageHandler;
        };
    } // namespace Platform
} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_MANAGER_H