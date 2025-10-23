#ifndef SPECTRUM_CPP_WINDOW_HELPERS_H
#define SPECTRUM_CPP_WINDOW_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Window Helpers - Utilities for Win32 window operations
//
// This header provides convenient, type-safe wrappers around common Win32
// window operations such as positioning, sizing, and state queries.
//
// Key features:
// - RAII-safe window rect operations
// - Screen-aware positioning utilities
// - Type-safe size/position calculations
// - Validation helpers
// - DPI-awareness support
//
// IMPORTANT: Function names are prefixed with "Window" to avoid conflicts
// with global Windows API functions (IsIconic, IsZoomed, etc.)
//
// Version: 1.0.1 (Fixed naming conflicts)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <windows.h>
#include <optional>

namespace Spectrum::Helpers::Window {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Type Definitions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct WindowRect {
        int left, top, right, bottom;

        [[nodiscard]] constexpr int Width() const noexcept {
            return right - left;
        }

        [[nodiscard]] constexpr int Height() const noexcept {
            return bottom - top;
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept {
            return Width() > 0 && Height() > 0;
        }
    };

    struct ScreenInfo {
        int width;
        int height;
        int workAreaLeft;
        int workAreaTop;
        int workAreaWidth;
        int workAreaHeight;

        [[nodiscard]] constexpr int WorkAreaRight() const noexcept {
            return workAreaLeft + workAreaWidth;
        }

        [[nodiscard]] constexpr int WorkAreaBottom() const noexcept {
            return workAreaTop + workAreaHeight;
        }
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Validates that HWND is not null and points to a valid window
    inline bool IsWindowValid(HWND hwnd) noexcept
    {
        return hwnd != nullptr && ::IsWindow(hwnd);
    }

    /// Validates window dimensions are within acceptable range
    inline bool IsValidSize(int width, int height) noexcept
    {
        constexpr int kMinSize = 1;
        constexpr int kMaxSize = 16384;
        return width >= kMinSize && width <= kMaxSize &&
            height >= kMinSize && height <= kMaxSize;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Gets window rectangle in screen coordinates
    inline std::optional<WindowRect> GetWindowRect(HWND hwnd) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return std::nullopt;
        }

        RECT rc;
        if (!::GetWindowRect(hwnd, &rc)) {
            return std::nullopt;
        }

        return WindowRect{ rc.left, rc.top, rc.right, rc.bottom };
    }

    /// Gets client area rectangle (coordinates relative to window)
    inline std::optional<WindowRect> GetClientRect(HWND hwnd) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return std::nullopt;
        }

        RECT rc;
        if (!::GetClientRect(hwnd, &rc)) {
            return std::nullopt;
        }

        return WindowRect{ rc.left, rc.top, rc.right, rc.bottom };
    }

    /// Gets screen dimensions and work area (excludes taskbar)
    inline ScreenInfo GetScreenInfo() noexcept
    {
        RECT workArea;
        ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

        return {
            ::GetSystemMetrics(SM_CXSCREEN),
            ::GetSystemMetrics(SM_CYSCREEN),
            workArea.left,
            workArea.top,
            workArea.right - workArea.left,
            workArea.bottom - workArea.top
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Positioning
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Calculates centered position for window on primary screen
    inline std::optional<POINT> CalculateCenterPosition(
        int windowWidth,
        int windowHeight
    ) noexcept
    {
        if (!IsValidSize(windowWidth, windowHeight)) {
            return std::nullopt;
        }

        const auto screen = GetScreenInfo();
        return POINT{
            (screen.width - windowWidth) / 2,
            (screen.height - windowHeight) / 2
        };
    }

    /// Centers window on screen
    inline bool CenterWindow(HWND hwnd) noexcept
    {
        auto rect = GetWindowRect(hwnd);
        if (!rect.has_value()) {
            return false;
        }

        auto centerPos = CalculateCenterPosition(rect->Width(), rect->Height());
        if (!centerPos.has_value()) {
            return false;
        }

        return ::SetWindowPos(
            hwnd,
            nullptr,
            centerPos->x,
            centerPos->y,
            0, 0,
            SWP_NOSIZE | SWP_NOZORDER
        ) != 0;
    }

    /// Positions window at bottom of screen (for overlays)
    inline bool PositionAtBottom(
        HWND hwnd,
        int height
    ) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return false;
        }

        const auto screen = GetScreenInfo();
        const int yPos = screen.height - height;

        const BOOL result = ::SetWindowPos(
            hwnd,
            HWND_TOPMOST,
            0,
            yPos,
            screen.width,
            height,
            SWP_SHOWWINDOW
        );

        if (result) {
            ::InvalidateRect(hwnd, nullptr, FALSE);
        }

        return result != 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries (Prefixed with "Window" to avoid WinAPI conflicts)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Checks if window is visible
    inline bool IsWindowVisibleState(HWND hwnd) noexcept
    {
        return IsWindowValid(hwnd) && ::IsWindowVisible(hwnd);
    }

    /// Checks if window is minimized (iconic)
    inline bool IsWindowMinimized(HWND hwnd) noexcept
    {
        return IsWindowValid(hwnd) && ::IsIconic(hwnd);
    }

    /// Checks if window is maximized (zoomed)
    inline bool IsWindowMaximized(HWND hwnd) noexcept
    {
        return IsWindowValid(hwnd) && ::IsZoomed(hwnd);
    }

    /// Checks if window is active and visible (not minimized)
    inline bool IsActiveAndVisible(HWND hwnd) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return false;
        }

        if (!IsWindowVisibleState(hwnd)) {
            return false;
        }

        if (IsWindowMinimized(hwnd)) {
            return false;
        }

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // DPI Awareness (Windows 10+)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Gets DPI for window (Windows 10, version 1607+)
    inline UINT GetDpiForWindow(HWND hwnd) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return USER_DEFAULT_SCREEN_DPI;
        }

        // Dynamically load function (requires Windows 10, version 1607+)
        using GetDpiForWindowFunc = UINT(WINAPI*)(HWND);
        static auto getDpiFunc = reinterpret_cast<GetDpiForWindowFunc>(
            ::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")
            );

        if (getDpiFunc) {
            return getDpiFunc(hwnd);
        }

        return USER_DEFAULT_SCREEN_DPI;
    }

    /// Scales value according to DPI
    template<typename T>
    [[nodiscard]] inline T ScaleForDpi(T value, UINT dpi) noexcept
    {
        return static_cast<T>(value * dpi / USER_DEFAULT_SCREEN_DPI);
    }

    /// Scales value for window's DPI
    template<typename T>
    [[nodiscard]] inline T ScaleForWindowDpi(HWND hwnd, T value) noexcept
    {
        const UINT dpi = GetDpiForWindow(hwnd);
        return ScaleForDpi(value, dpi);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Shows window with specified show command
    inline bool ShowWindowState(HWND hwnd, int cmdShow = SW_SHOW) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return false;
        }

        return ::ShowWindow(hwnd, cmdShow) != 0;
    }

    /// Hides window
    inline bool HideWindow(HWND hwnd) noexcept
    {
        return ShowWindowState(hwnd, SW_HIDE);
    }

    /// Brings window to foreground and activates it
    inline bool BringToFront(HWND hwnd) noexcept
    {
        if (!IsWindowValid(hwnd)) {
            return false;
        }

        return ::SetForegroundWindow(hwnd) != 0;
    }

} // namespace Spectrum::Helpers::Window

#endif // SPECTRUM_CPP_WINDOW_HELPERS_H