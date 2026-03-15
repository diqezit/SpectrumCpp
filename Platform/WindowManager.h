#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Manages main/overlay/UI windows and their render engines.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <functional>
#include <memory>

namespace Spectrum {
    class ControllerCore;
    class EventBus;
    class RenderEngine;
    class UIManager;

    namespace Platform {
        class MainWindow;
        class UIWindow;
        class MessageHandler;
        class UIMessageHandler;

        inline constexpr Color kUIBackgroundColor =
            Color::FromRGB(30, 30, 40);

        class WindowManager final {
        public:
            WindowManager(HINSTANCE, ControllerCore*, EventBus*);
            ~WindowManager() noexcept;

            WindowManager(const WindowManager&) = delete;
            WindowManager& operator=(const WindowManager&) = delete;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Lifecycle
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            [[nodiscard]] bool Initialize();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            [[nodiscard]] bool HandleVisualizationResize(
                int w, int h, bool recreate = false);
            [[nodiscard]] bool HandleUIResize(
                int w, int h, bool recreate = false);

            void OnResizeStart();
            void OnResizeEnd(HWND);
            void OnResize(HWND, int w, int h);

            void OnUIResizeStart();
            void OnUIResizeEnd(HWND);
            void OnUIResize(HWND, int w, int h);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Window operations
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            void ToggleOverlay();
            void ShowUIWindow() const;
            void HideUIWindow() const;
            void ForceUIRender();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Queries
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            [[nodiscard]] bool IsRunning() const;
            [[nodiscard]] bool IsActive() const;
            [[nodiscard]] bool IsUIWindowVisible() const;

            [[nodiscard]] bool IsOverlayMode() const noexcept {
                return m_isOverlay;
            }

            [[nodiscard]] bool IsResizing() const noexcept {
                return m_viz.resizing || m_ui.resizing;
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Accessors
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            [[nodiscard]] RenderEngine* GetVisualizationEngine() const noexcept {
                return m_viz.engine.get();
            }

            [[nodiscard]] RenderEngine* GetUIEngine() const noexcept {
                return m_ui.engine.get();
            }

            [[nodiscard]] UIManager* GetUIManager() const noexcept {
                return m_uiManager.get();
            }

            [[nodiscard]] MessageHandler* GetMessageHandler() const noexcept {
                return m_msgHandler.get();
            }

            [[nodiscard]] MainWindow* GetMainWindow() const noexcept {
                return m_mainWnd.get();
            }

            [[nodiscard]] UIWindow* GetUIWindow() const noexcept {
                return m_uiWnd.get();
            }

            [[nodiscard]] HWND GetCurrentHwnd() const;
            [[nodiscard]] HWND GetUIHwnd() const noexcept;

        private:
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Engine slot — shared state for viz / UI
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            struct EngineSlot {
                std::unique_ptr<RenderEngine> engine;
                bool resizing = false;
                int  lastW = 0;
                int  lastH = 0;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Initialization
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            bool InitializeWindows();
            bool InitializeGraphics();
            bool InitializeUI();

            std::unique_ptr<MainWindow> CreateMainWnd(
                const wchar_t* title, int w, int h, bool overlay) const;
            std::unique_ptr<UIWindow> CreateUIWnd() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Engine management (DRY)
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            bool RecreateEngine(
                EngineSlot& slot, HWND hwnd,
                bool overlay, bool d2dOnly);

            void OnSlotResize(
                EngineSlot& slot, int w, int h,
                const std::function<bool(int, int)>& handler);

            void OnSlotResizeEnd(
                EngineSlot& slot, HWND hwnd,
                const std::function<bool(int, int)>& handler);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Overlay
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            void SwitchActiveWindow(MainWindow* hide, MainWindow* show);
            void PositionOverlayWindow() const;
            void NotifyRendererOfModeChange() const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // State
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            HINSTANCE       m_hInstance;
            ControllerCore* m_controller;
            bool            m_isOverlay = false;

            // Windows
            std::unique_ptr<MainWindow>       m_mainWnd;
            std::unique_ptr<MainWindow>       m_overlayWnd;
            std::unique_ptr<UIWindow>         m_uiWnd;

            // Engine slots
            EngineSlot m_viz;
            EngineSlot m_ui;

            // Handlers
            std::unique_ptr<MessageHandler>   m_msgHandler;
            std::unique_ptr<UIMessageHandler> m_uiMsgHandler;
            std::unique_ptr<UIManager>        m_uiManager;
        };
    }
}

#endif