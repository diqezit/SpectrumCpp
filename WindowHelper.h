// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// A collection of helper functions for common Win32 window operations,
// such as creating windows with specific styles, applying overlay effects,
// and calculating window and screen dimensions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_WINDOW_HELPER_H
#define SPECTRUM_CPP_WINDOW_HELPER_H

#include "Common.h"

namespace Spectrum {
    namespace WindowUtils {

        struct Styles {
            DWORD style;
            DWORD exStyle;
        };

        Styles MakeStyles(bool overlay);

        void AdjustRectIfNeeded(RECT& rect,
            const Styles& st,
            bool overlay);

        bool RegisterWindowClass(HINSTANCE hInstance,
            const wchar_t* className,
            WNDPROC wndProc,
            bool overlay);

        HWND CreateWindowWithStyles(HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* title,
            const Styles& st,
            int x,
            int y,
            int w,
            int h,
            void* userPtr);

        void ApplyOverlay(HWND hwnd);

        void ShowAndUpdate(HWND hwnd);

        void ExtractMouse(LPARAM lParam,
            int& x,
            int& y);

        void ExtractSize(LPARAM lParam,
            int& w,
            int& h);

        void UpdateMinimizeFlagOnSize(WPARAM wParam,
            std::atomic<bool>& minimized);

        struct Size { int w, h; };
        Size GetScreenSize();
        Size GetWindowSize(HWND hwnd);

        struct Pos { int x, y; };
        Pos CalculateCenterPosition(const Size& windowSize, const Size& screenSize);

        void CenterOnScreen(HWND hwnd);

    } // namespace WindowUtils
} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_HELPER_H