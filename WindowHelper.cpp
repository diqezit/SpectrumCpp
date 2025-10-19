// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Implements the helper functions for Win32 window management,
// providing low-level support for window creation and manipulation.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "WindowHelper.h"

namespace Spectrum {
    namespace WindowUtils {

        namespace {

            void FillWindowClass(WNDCLASSEXW& wc,
                HINSTANCE hInstance,
                const wchar_t* className,
                WNDPROC wndProc,
                bool overlay) {
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
            }

            bool HandleClassRegistrationError() {
                DWORD err = GetLastError();
                if (err == ERROR_CLASS_ALREADY_EXISTS) return true;

                LOG_ERROR("Failed to register window class: " << err);
                return false;
            }

        }

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
            FillWindowClass(wc, hInstance, className, wndProc, overlay);

            if (RegisterClassExW(&wc)) return true;
            return HandleClassRegistrationError();
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

        Size GetScreenSize() {
            return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        }

        Size GetWindowSize(HWND hwnd) {
            RECT rect{};
            if (!GetWindowRect(hwnd, &rect)) return { 0, 0 };
            return { rect.right - rect.left, rect.bottom - rect.top };
        }

        Pos CalculateCenterPosition(const Size& windowSize, const Size& screenSize) {
            return { (screenSize.w - windowSize.w) / 2, (screenSize.h - windowSize.h) / 2 };
        }

        void CenterOnScreen(HWND hwnd) {
            Size windowSize = GetWindowSize(hwnd);
            if (windowSize.w == 0 && windowSize.h == 0) return;

            Size screenSize = GetScreenSize();
            Pos centerPos = CalculateCenterPosition(windowSize, screenSize);

            SetWindowPos(hwnd,
                nullptr,
                centerPos.x,
                centerPos.y,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);
        }

    } // namespace WindowUtils
} // namespace Spectrum