// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the MainWindow class, a RAII-compliant C++ wrapper around a
// native Win32 window (HWND).
//
// The MainWindow encapsulates the complete lifecycle of a Win32 window,
// including window class registration, window creation, message processing,
// and proper cleanup. It provides a high-level, type-safe interface that
// abstracts the complexities of Win32 API, delegating application-specific
// message handling to a WindowManager instance.
//
// Key Responsibilities:
// - Register and unregister window classes
// - Create and destroy window handles
// - Process Windows messages through a message pump
// - Maintain window state (running, dimensions, overlay mode)
// - Delegate message handling to WindowManager via static WndProc
//
// RAII Guarantees:
// - Window handle is destroyed in destructor
// - Window class is unregistered in destructor
// - Non-copyable and non-movable to prevent resource duplication
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common.h"
#include "WindowHelper.h"
#include <string>

namespace Spectrum {

    class WindowManager;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // MainWindow Class
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class MainWindow final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit MainWindow(HINSTANCE hInstance);
        ~MainWindow() noexcept;

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator=(const MainWindow&) = delete;
        MainWindow(MainWindow&&) = delete;
        MainWindow& operator=(MainWindow&&) = delete;

        [[nodiscard]] bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            bool isOverlay,
            void* userPtr
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ProcessMessages();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Show(int cmdShow = SW_SHOW) const;
        void Hide() const;
        void Close();
        void SetRunning(bool running) noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsRunning() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] HWND GetHwnd() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct WindowRectParams {
            int x;
            int y;
            int w;
            int h;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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
        void Cleanup() noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Win32 Message Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static LRESULT CALLBACK WndProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

        static void StoreManagerPointer(HWND hwnd, LPARAM lParam);
        static WindowManager* GetManagerFromHwnd(HWND hwnd);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        std::wstring m_className;

        bool m_running;
        bool m_isOverlay;
        bool m_classRegistered;

        int m_width;
        int m_height;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_MAINWINDOW_H