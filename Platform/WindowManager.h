#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

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

        constexpr Color kUIBackgroundColor = Color::FromRGB(30, 30, 40);

        class WindowManager final {
        public:
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

            [[nodiscard]] bool HandleVisualizationResize(int width, int height, bool recreateContext = false);
            [[nodiscard]] bool HandleUIResize(int width, int height, bool recreateContext = false);

            void OnResizeStart();
            void OnResizeEnd(HWND hwnd);
            void OnResize(HWND hwnd, int width, int height);

            void OnUIResizeStart();
            void OnUIResizeEnd(HWND hwnd);
            void OnUIResize(HWND hwnd, int width, int height);

            void ToggleOverlay();
            void ShowUIWindow() const;
            void HideUIWindow() const;
            void ForceUIRender();

            [[nodiscard]] bool IsRunning() const;
            [[nodiscard]] bool IsOverlayMode() const noexcept;
            [[nodiscard]] bool IsActive() const;
            [[nodiscard]] bool IsResizing() const noexcept;
            [[nodiscard]] bool IsUIWindowVisible() const;

            [[nodiscard]] RenderEngine* GetVisualizationEngine() const noexcept;
            [[nodiscard]] RenderEngine* GetUIEngine() const noexcept;
            [[nodiscard]] UIManager* GetUIManager() const noexcept;
            [[nodiscard]] MessageHandler* GetMessageHandler() const noexcept;
            [[nodiscard]] MainWindow* GetMainWindow() const noexcept;
            [[nodiscard]] UIWindow* GetUIWindow() const noexcept;
            [[nodiscard]] HWND GetCurrentHwnd() const;

        private:
            bool InitializeWindows();
            bool InitializeGraphics();
            bool InitializeUI();
            void WarmupUI();

            [[nodiscard]] std::unique_ptr<MainWindow> CreateMainWindowInstance(
                const wchar_t* title, int width, int height, bool isOverlay) const;
            [[nodiscard]] std::unique_ptr<UIWindow> CreateUIWindowInstance() const;

            void SwitchActiveWindow(MainWindow* hide, MainWindow* show);
            void ShowWindow(MainWindow* window) const;
            void HideWindow(MainWindow* window) const;
            void PositionOverlayWindow() const;

            bool RecreateVisualizationContext(HWND hwnd);
            bool RecreateUIContext(HWND hwnd);
            void NotifyRendererOfModeChange() const;

            [[nodiscard]] bool ShouldSkipResize(int width, int height) const noexcept;

            template <typename TEngine>
            bool HandleResizeInternal(
                TEngine& engine,
                int width, int height,
                bool recreateContext,
                const char* logContext,
                const std::function<bool()>& recreateFunc,
                const std::function<void(int, int)>& notifyFunc);

            void OnResizeEndInternal(
                HWND hwnd,
                bool& isResizingFlag,
                const char* logContext,
                const std::function<bool(int, int)>& resizeHandler);

            void OnResizeInternal(
                HWND hwnd,
                int width, int height,
                int& lastWidth, int& lastHeight,
                bool isResizing,
                RenderEngine* engine,
                const std::function<bool(int, int)>& handler);

        private:
            HINSTANCE m_hInstance;
            ControllerCore* m_controller;

            bool m_isOverlay;
            bool m_isResizing;
            bool m_isUIResizing;

            int m_lastWidth;
            int m_lastHeight;
            int m_lastUIWidth;
            int m_lastUIHeight;

            std::unique_ptr<MainWindow> m_mainWnd;
            std::unique_ptr<MainWindow> m_overlayWnd;
            std::unique_ptr<RenderEngine> m_engine;
            std::unique_ptr<MessageHandler> m_messageHandler;

            std::unique_ptr<UIWindow> m_uiWnd;
            std::unique_ptr<RenderEngine> m_uiEngine;
            std::unique_ptr<UIManager> m_uiManager;
            std::unique_ptr<UIMessageHandler> m_uiMessageHandler;
        };

    }
}

#endif