// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the MainWindow class, a high-level wrapper around a
// native Win32 window (HWND). It handles window creation, registration,
// the message loop, and delegates message handling to the controller
//
// Defines the MainWindow, a RAII-compliant C++ wrapper for a Win32 HWND. It
// encapsulates the complexities of window class registration, creation, and
// message processing, delegating application-specific logic to a designated
// message handler.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common.h"
#include "WindowHelper.h"
#include <atomic>

namespace Spectrum {

    // Forward declarations
    class WindowManager;

    class MainWindow final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        explicit MainWindow(HINSTANCE hInstance);
        ~MainWindow() noexcept;

        [[nodiscard]] bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            bool isOverlay,
            void* userPtr
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void ProcessMessages();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // State Queries & Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void Show(int cmdShow = SW_SHOW) const;
        void Hide() const;
        void Close();

        [[nodiscard]] bool IsRunning() const noexcept;
        void SetRunning(bool running);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] HWND GetHwnd() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct WindowRectParams {
            int x, y, w, h;
        };

        [[nodiscard]] bool RegisterWindowClass();
        [[nodiscard]] WNDCLASSEXW CreateWindowClass() const;
        [[nodiscard]] bool CreateAndConfigureWindow(
            const std::wstring& title,
            int width,
            int height,
            void* userPtr
        );
        [[nodiscard]] WindowRectParams CalculateWindowRect(
            int width,
            int height,
            const WindowUtils::Styles& styles
        ) const;

        void ApplyPostCreationStyles() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Win32 Message Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        static LRESULT CALLBACK WndProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );
        static void StoreManagerPointer(HWND hwnd, LPARAM lParam);
        static WindowManager* GetManagerFromHwnd(HWND hwnd);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        std::wstring m_className;
        std::atomic<bool> m_running;
        bool m_isOverlay;
        int m_width;
        int m_height;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_MAINWINDOW_H