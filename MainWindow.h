// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// MainWindow.h: Defines the MainWindow class for handling main and overlay windows.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common.h"

namespace Spectrum {

    class MainWindow {
    public:
        using KeyCallback = std::function<void(int key)>;
        using MouseMoveCallback = std::function<void(int x, int y)>;
        using MouseClickCallback = std::function<void(int x, int y)>;
        using ResizeCallback = std::function<void(int width, int height)>;
        using CloseCallback = std::function<void()>;

        explicit MainWindow(HINSTANCE hInstance);
        ~MainWindow();

        bool Initialize(bool isOverlay = false,
            int width = 800,
            int height = 600);

        void ProcessMessages();

        void Show() const;
        void Hide() const;
        void Close();
        void Minimize();
        void Restore();
        void SetTitle(const std::wstring& title);
        void SetPosition(int x, int y) const;
        void CenterOnScreen() const;
        void MakeTransparent() const;
        void MakeOpaque() const;

        HWND GetHwnd() const { return m_hwnd; }
        int  GetWidth() const { return m_width; }
        int  GetHeight() const { return m_height; }
        bool IsRunning() const { return m_running; }
        bool IsActive() const { return !m_isMinimized && m_running; }
        bool IsOverlay() const { return m_isOverlay; }

        void SetKeyCallback(KeyCallback cb) { m_keyCallback = std::move(cb); }
        void SetMouseMoveCallback(MouseMoveCallback cb)
        {
            m_mouseMoveCallback = std::move(cb);
        }
        void SetMouseClickCallback(MouseClickCallback cb)
        {
            m_mouseClickCallback = std::move(cb);
        }
        void SetResizeCallback(ResizeCallback cb)
        {
            m_resizeCallback = std::move(cb);
        }
        void SetCloseCallback(CloseCallback cb)
        {
            m_closeCallback = std::move(cb);
        }

        static LRESULT CALLBACK WindowProc(HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam);

    private:
        bool RegisterWindowClass() const;
        bool CreateWindowInstance();
        void ApplyWindowStyles() const;

        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        void HandleResize(WPARAM wParam, LPARAM lParam);
        void HandleKeyDown(WPARAM wParam);
        void HandleMouseMove(LPARAM lParam);
        void HandleMouseClick(LPARAM lParam);
        void HandleClose();

        DWORD GetWindowStyleFlags() const;
        DWORD GetWindowExStyleFlags() const;
        void UpdateWindowStyles() const;

    private:
        HINSTANCE m_hInstance;
        HWND      m_hwnd;
        int       m_width;
        int       m_height;
        bool      m_isOverlay;
        std::atomic<bool> m_running;
        std::atomic<bool> m_isMinimized;
        std::wstring m_className;
        std::wstring m_title;

        KeyCallback        m_keyCallback;
        MouseMoveCallback  m_mouseMoveCallback;
        MouseClickCallback m_mouseClickCallback;
        ResizeCallback     m_resizeCallback;
        CloseCallback      m_closeCallback;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_MAINWINDOW_H