// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// WindowHelper.h: Common helpers for Win32 window creation and styles.
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

        void GetScreenSize(int& w,
            int& h);

        void CenterOnScreen(HWND hwnd);

    } // namespace WindowUtils
} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_HELPER_H