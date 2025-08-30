// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// WindowHelper.cpp: Implementation of common Win32 window helpers.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "WindowHelper.h"
#include "Utils.h"

namespace Spectrum {
    namespace WindowUtils {

        Styles MakeStyles(bool overlay) {
            if (overlay)
                return { WS_POPUP,
                         WS_EX_LAYERED | WS_EX_TRANSPARENT |
                         WS_EX_TOPMOST | WS_EX_TOOLWINDOW };
            return { WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW };
        }

        void AdjustRectIfNeeded(RECT& rect,
            const Styles& st,
            bool overlay) {
            if (overlay) return;
            AdjustWindowRectEx(&rect, st.style, FALSE, st.exStyle);
        }

        bool RegisterWindowClass(HINSTANCE hInstance,
            const wchar_t* className,
            WNDPROC wndProc,
            bool overlay) {
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = wndProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = hInstance;
            wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = overlay
                ? nullptr
                : (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszMenuName = nullptr;
            wc.lpszClassName = className;
            wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

            if (RegisterClassExW(&wc)) return true;

            DWORD err = GetLastError();
            if (err == ERROR_CLASS_ALREADY_EXISTS) return true;

            LOG_ERROR("Failed to register window class: " << err);
            return false;
        }

        HWND CreateWindowWithStyles(HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* title,
            const Styles& st,
            int x,
            int y,
            int w,
            int h,
            void* userPtr) {
            return CreateWindowExW(
                st.exStyle,
                className,
                title,
                st.style,
                x,
                y,
                w,
                h,
                nullptr,
                nullptr,
                hInstance,
                userPtr
            );
        }

        void ApplyOverlay(HWND hwnd) {
            SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
        }

        void ShowAndUpdate(HWND hwnd) {
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }

        void ExtractMouse(LPARAM lParam,
            int& x,
            int& y) {
            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);
        }

        void ExtractSize(LPARAM lParam,
            int& w,
            int& h) {
            w = LOWORD(lParam);
            h = HIWORD(lParam);
        }

        void UpdateMinimizeFlagOnSize(WPARAM wParam,
            std::atomic<bool>& minimized) {
            switch (wParam) {
            case SIZE_MINIMIZED: minimized = true; break;
            case SIZE_RESTORED:
            case SIZE_MAXIMIZED: minimized = false; break;
            default: break;
            }
        }

        void GetScreenSize(int& w,
            int& h) {
            w = GetSystemMetrics(SM_CXSCREEN);
            h = GetSystemMetrics(SM_CYSCREEN);
        }

        void CenterOnScreen(HWND hwnd) {
            RECT rect{};
            if (!GetWindowRect(hwnd, &rect)) return;

            int ww = rect.right - rect.left;
            int hh = rect.bottom - rect.top;

            int sw = GetSystemMetrics(SM_CXSCREEN);
            int sh = GetSystemMetrics(SM_CYSCREEN);

            int x = (sw - ww) / 2;
            int y = (sh - hh) / 2;

            SetWindowPos(hwnd,
                nullptr,
                x,
                y,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);
        }

    } // namespace WindowUtils
} // namespace Spectrum