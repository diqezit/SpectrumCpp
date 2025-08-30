// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// WindowManager.h: Centralized window management and overlay coordination.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

#include "Common.h"
#include "MainWindow.h"
#include "OverlayWindow.h"
#include "GraphicsContext.h"
#include "ColorPicker.h"

namespace Spectrum {

    class WindowManager {
    public:
        enum class Mode {
            Normal,
            Overlay,
            Transitioning
        };

        using KeyCallback = std::function<void(int)>;
        using MouseMoveCallback = std::function<void(int, int)>;
        using MouseClickCallback = std::function<void(int, int)>;
        using ResizeCallback = std::function<void(int, int)>;
        using CloseCallback = std::function<void()>;

        explicit WindowManager(HINSTANCE hInstance);
        ~WindowManager();

        bool Initialize(int width = 800, int height = 600);

        void ProcessMessages();

        bool ToggleOverlay();
        bool EnterOverlayMode();
        bool ExitOverlayMode();

        bool IsRunning() const;
        bool IsActive() const;
        bool IsOverlayMode() const { return m_mode == Mode::Overlay; }

        void SetTitle(const std::wstring& title);
        void CenterWindow();

        GraphicsContext* GetGraphics() { return m_graphics.get(); }
        ColorPicker* GetColorPicker() { return m_colorPicker.get(); }
        MainWindow* GetMainWindow() { return m_mainWindow.get(); }

        void SetKeyCallback(KeyCallback cb) { m_keyCallback = std::move(cb); }
        void SetMouseMoveCallback(MouseMoveCallback cb) { m_mouseMoveCallback = std::move(cb); }
        void SetMouseClickCallback(MouseClickCallback cb) { m_mouseClickCallback = std::move(cb); }
        void SetResizeCallback(ResizeCallback cb) { m_resizeCallback = std::move(cb); }
        void SetCloseCallback(CloseCallback cb) { m_closeCallback = std::move(cb); }

    private:
        bool CreateMainWindow(int width, int height);
        bool InitializeGraphics();
        bool InitializeColorPicker();
        void SetupMainWindowCallbacks();

        void NotifyResize(int width, int height);

    private:
        HINSTANCE m_hInstance;
        Mode m_mode;

        std::unique_ptr<MainWindow> m_mainWindow;
        std::unique_ptr<OverlayWindow> m_overlayWindow;

        std::unique_ptr<GraphicsContext> m_graphics;
        std::unique_ptr<ColorPicker> m_colorPicker;

        KeyCallback m_keyCallback;
        MouseMoveCallback m_mouseMoveCallback;
        MouseClickCallback m_mouseClickCallback;
        ResizeCallback m_resizeCallback;
        CloseCallback m_closeCallback;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_MANAGER_H