// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the helper functions for Win32 window management,
// providing low-level support for window creation and manipulation.
//
// Implements the WindowUtils helpers, providing concrete, low-level
// definitions for creating styled windows, applying visual effects, and
// calculating screen and window geometry.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WindowHelper.h"

namespace Spectrum {
    namespace WindowUtils {

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Window Creation & Styling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] Styles MakeStyles(bool isOverlay)
        {
            if (isOverlay)
                return {
                    WS_POPUP,
                    WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW  // no WS_EX_TRANSPARENT
            };
            return { WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW };
        }

        void AdjustRectIfNeeded(
            RECT& rect,
            const Styles& styles,
            bool isOverlay
        )
        {
            if (isOverlay) return;
            AdjustWindowRectEx(&rect, styles.style, FALSE, styles.exStyle);
        }

        HWND CreateWindowWithStyles(
            HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* title,
            const Styles& styles,
            int x,
            int y,
            int w,
            int h,
            void* userPtr
        )
        {
            return CreateWindowExW(
                styles.exStyle,
                className,
                title,
                styles.style,
                x, y, w, h,
                nullptr, // No parent window
                nullptr, // No menu
                hInstance,
                userPtr
            );
        }

        void ApplyOverlay(HWND hwnd)
        {
            SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Window State & Position
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void CenterOnScreen(HWND hwnd)
        {
            const Size windowSize = GetWindowSize(hwnd);
            if (windowSize.w == 0 && windowSize.h == 0) return;

            const Size screenSize = GetScreenSize();
            const Pos centerPos = CalculateCenterPosition(windowSize, screenSize);

            SetWindowPos(
                hwnd,
                nullptr,
                centerPos.x,
                centerPos.y,
                0, 0, // width, height (no change)
                SWP_NOSIZE | SWP_NOZORDER
            );
        }

        [[nodiscard]] Size GetScreenSize()
        {
            return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        }

        [[nodiscard]] Size GetWindowSize(HWND hwnd)
        {
            RECT rect{};
            if (!GetWindowRect(hwnd, &rect)) return { 0, 0 };
            return { rect.right - rect.left, rect.bottom - rect.top };
        }

        [[nodiscard]] Pos CalculateCenterPosition(const Size& windowSize, const Size& screenSize)
        {
            return {
                (screenSize.w - windowSize.w) / 2,
                (screenSize.h - windowSize.h) / 2
            };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Message & Parameter Extraction
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void ExtractMousePos(LPARAM lParam, int& x, int& y)
        {
            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);
        }

        void ExtractSize(LPARAM lParam, int& w, int& h)
        {
            w = LOWORD(lParam);
            h = HIWORD(lParam);
        }

    } // namespace WindowUtils
} // namespace Spectrum