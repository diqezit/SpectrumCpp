// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the MainWindow class, a RAII-compliant C++ wrapper around a
// native Win32 window (HWND).
//
// The MainWindow encapsulates the complete lifecycle of a Win32 window,
// including class registration, window creation, and cleanup. It delegates
// application-specific message handling to a MessageHandler instance via a
// static WndProc, serving as a pure resource management class.
//
// Refactored to follow SRP and DRY principles with small, focused functions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common/Common.h"
#include <string>

namespace Spectrum::Platform {

    class MessageHandler;

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
            MessageHandler* messageHandler
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

        struct WindowIcons {
            HICON largeIcon;   // Renamed from 'large' to avoid conflicts
            HICON smallIcon;   // Renamed from 'small' to avoid Windows macro conflict
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization - High Level
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        bool ValidateInitializationParams(int width, int height, MessageHandler* handler) const;
        void StoreWindowDimensions(int width, int height) noexcept;
        void SetupWindowConfiguration(bool isOverlay);
        bool RegisterWindowClass();
        bool CreateAndConfigureWindow(
            const std::wstring& title,
            int width,
            int height,
            MessageHandler* messageHandler
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Window Class Registration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] WNDCLASSEXW CreateWindowClass() const;
        void ConfigureWindowClassBase(WNDCLASSEXW& wcex) const;
        void SetWindowClassStyle(WNDCLASSEXW& wcex) const;
        void SetWindowClassBackground(WNDCLASSEXW& wcex) const;
        [[nodiscard]] WindowIcons LoadWindowIcons() const;
        void ApplyWindowIcons(WNDCLASSEXW& wcex, const WindowIcons& icons) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Window Creation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] HWND CreateWindowHandle(
            const std::wstring& title,
            const WindowRectParams& params,
            DWORD style,
            DWORD exStyle,
            MessageHandler* messageHandler
        );
        bool ValidateWindowHandle(HWND hwnd) const;
        void ConfigureNewWindow();
        [[nodiscard]] WindowRectParams CalculateWindowRect(
            int width,
            int height,
            DWORD style,
            DWORD exStyle
        ) const;
        [[nodiscard]] WindowRectParams CalculateOverlayRect(int width, int height) const;
        [[nodiscard]] WindowRectParams CalculateNormalRect(
            int width,
            int height,
            DWORD style,
            DWORD exStyle
        ) const;
        void ApplyPostCreationStyles() const;
        void ApplyOverlayTransparency() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Cleanup
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Cleanup() noexcept;
        void DestroyWindowSafe() noexcept;
        void UnregisterClassSafe() noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ValidateHInstance() const noexcept;
        [[nodiscard]] bool ValidateDimensions(int width, int height) const noexcept;
        [[nodiscard]] bool ValidateMessageHandler(MessageHandler* handler) const noexcept;
        [[nodiscard]] bool ValidateClassName() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Win32 Message Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static void StoreMessageHandlerPointer(HWND hwnd, LPARAM lParam);
        static MessageHandler* GetMessageHandlerFromHwnd(HWND hwnd);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helpers - Static to avoid const issues
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::wstring GenerateClassName(bool isOverlay) const;
        [[nodiscard]] static HICON LoadApplicationIcon(HINSTANCE hInstance, bool isSmallIcon);
        [[nodiscard]] static HICON LoadDefaultIcon(bool isSmallIcon);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Logging
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void LogInitialization(const std::wstring& title, int width, int height) const;
        void LogWindowCreated() const;
        void LogCleanup() const;
        void LogError(const char* operation) const;

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

} // namespace Spectrum::Platform

#endif // SPECTRUM_CPP_MAINWINDOW_H