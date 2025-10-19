// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the MainWindow class, a high-level wrapper around a
// native Win32 window (HWND). It handles window creation, registration,
// the message loop, and delegates message handling to the controller
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common.h"
#include "WindowHelper.h"

namespace Spectrum {

    class WindowManager;

    class MainWindow {
    public:
        explicit MainWindow(HINSTANCE hInstance);
        ~MainWindow();

        bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            bool isOverlay,
            void* userPtr
        );

        void ProcessMessages();
        void Show(int cmdShow = SW_SHOW) const;
        void Hide() const;
        void Close();

        HWND GetHwnd() const { return m_hwnd; }
        bool IsRunning() const { return m_running; }
        void SetRunning(bool running) { m_running = running; }

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }

    private:
        bool Register();
        WNDCLASSEXW CreateWindowClass() const;
        bool CreateWindowInstance(
            const std::wstring& title,
            int width,
            int height,
            void* userPtr
        );
        struct WindowRectParams { int x, y, w, h; };
        WindowRectParams CalculateWindowRect(
            int width,
            int height,
            const WindowUtils::Styles& styles
        ) const;

        void ApplyStyles() const;

        static LRESULT CALLBACK WndProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );
        static void StoreManagerPointer(HWND hwnd, LPARAM lParam);
        static WindowManager* GetManagerFromHwnd(HWND hwnd);

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        std::wstring m_className;
        std::atomic<bool> m_running;
        bool m_isOverlay;
        int m_width;
        int m_height;
    };

} // namespace Spectrum

#endif