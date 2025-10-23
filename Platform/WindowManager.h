#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the WindowManager for window lifecycle orchestration.
//
// This class serves as the central coordinator for all windowing operations,
// managing the complete lifecycle of both normal and overlay windows,
// handling mode transitions, and coordinating graphics context recreation.
//
// Key features:
// - Dual-mode operation (normal window / screen overlay)
// - Automatic graphics context recreation on mode switch
// - Resize propagation to all subsystems with debouncing
// - Message processing delegation to MessageHandler
// - Centralized window state management
//
// Design notes:
// - Owns RenderEngine, UIManager, MessageHandler instances
// - Non-owning pointer to ControllerCore (parent)
// - Validates dependencies at construction time
// - All mode transitions logged for debugging
// - Resize events are debounced to optimize performance
// - Refactored to use WindowHelpers.h for cleaner operations
// - Functions follow SRP and DRY principles
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <memory>
#include <chrono>

namespace Spectrum {
    class ControllerCore;
    class EventBus;
    class RenderEngine;
    class UIManager;

    namespace Platform {
        class MainWindow;
        class MessageHandler;

        class WindowManager final
        {
        public:
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
            // Main Execution
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void ProcessMessages();

            void PropagateResizeToSubsystems(HWND hwnd);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Resize Management (Optimized)
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void OnResizeStart();
            void OnResizeEnd(HWND hwnd);
            void OnResize(HWND hwnd, int width, int height);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Mode Management
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void ToggleOverlay();

            [[nodiscard]] bool RecreateGraphicsAndNotify(HWND hwnd);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // State Queries
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] bool IsRunning() const;
            [[nodiscard]] bool IsOverlayMode() const noexcept;
            [[nodiscard]] bool IsActive() const;
            [[nodiscard]] bool IsResizing() const noexcept;

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
            // Window Initialization - High Level
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            bool InitializeMainWindow();
            bool InitializeOverlayWindow();
            bool InitializeGraphics();
            bool InitializeUIComponents();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Window Initialization - Low Level
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] std::unique_ptr<MainWindow> CreateMainWindowInstance() const;
            [[nodiscard]] std::unique_ptr<MainWindow> CreateOverlayWindowInstance() const;

            bool ConfigureMainWindow(MainWindow* window);
            bool ConfigureOverlayWindow(MainWindow* window);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Mode Transitions
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void ActivateOverlayMode();
            void DeactivateOverlayMode();
            void SwitchToMainWindow();
            void SwitchToOverlayWindow();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Window Operations
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void HideMainWindow() const;
            void ShowMainWindow() const;
            void HideOverlayWindow() const;
            void ShowOverlayWindow() const;

            void CenterMainWindow();
            void PositionOverlayWindow() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Graphics Management - High Level
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            bool RecreateGraphicsContext(HWND hwnd);
            bool RecreateUIResources();
            void NotifyRendererOfModeChange() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Graphics Management - Low Level
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] std::unique_ptr<RenderEngine> CreateRenderEngine(HWND hwnd) const;
            bool InitializeRenderEngine(RenderEngine* engine) const;
            bool UpdateUIResourcesFromEngine();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Resize Handling (Internal)
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void PerformLightweightResize(int width, int height);
            void PerformFullResize(HWND hwnd, int width, int height);

            void ResizeRenderEngine(int width, int height);
            void ResizeUIManager(int width, int height);
            void NotifyControllerResize(int width, int height);

            bool ShouldSkipResize(int width, int height) const noexcept;
            bool ExtractResizeDimensions(HWND hwnd, int& outWidth, int& outHeight) const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Calculation Helpers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] int CalculateOverlayHeight() const;
            [[nodiscard]] Point CalculateMainWindowCenter() const;
            [[nodiscard]] int GetScreenWidth() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Validation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] bool ValidateDependencies() const noexcept;
            [[nodiscard]] bool ValidateController() const noexcept;
            [[nodiscard]] bool ValidateUIManager() const noexcept;
            [[nodiscard]] bool ValidateRenderEngine() const noexcept;
            [[nodiscard]] bool ValidateWindow(HWND hwnd) const noexcept;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Logging Helpers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void LogModeSwitch(bool toOverlay) const;
            void LogWindowInitialization(const wchar_t* windowType) const;
            void LogGraphicsRecreation() const;
            void LogWindowCreated(const wchar_t* windowType, int width, int height) const;
            void LogResizeOperation(const char* operationType, int width, int height) const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Member Variables
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            HINSTANCE m_hInstance;
            ControllerCore* m_controller; // Non-owning

            bool m_isOverlay;
            bool m_isResizing;

            std::unique_ptr<MainWindow> m_mainWnd;
            std::unique_ptr<MainWindow> m_overlayWnd;
            std::unique_ptr<RenderEngine> m_engine;
            std::unique_ptr<UIManager> m_uiManager;
            std::unique_ptr<MessageHandler> m_messageHandler;

            // Resize optimization
            int m_lastWidth;
            int m_lastHeight;
        };

    } // namespace Platform
} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_MANAGER_H