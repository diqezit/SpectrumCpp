// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// A collection of helper functions for common Win32 window operations,
// such as creating windows with specific styles, applying overlay effects,
// and calculating window and screen dimensions.
//
// Defines the WindowUtils namespace, a library of pure, stateless helper
// functions that abstract low-level Win32 API calls for window and display
// manipulation. This promotes code reuse and isolates platform-specific
// details.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WINDOW_HELPER_H
#define SPECTRUM_CPP_WINDOW_HELPER_H

#include "Common.h"
#include <atomic>

namespace Spectrum {
    namespace WindowUtils {

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Type Definitions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct Styles {
            DWORD style;
            DWORD exStyle;
        };

        struct Size {
            int w, h;
        };

        struct Pos {
            int x, y;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Window Creation & Styling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] Styles MakeStyles(bool isOverlay);

        void AdjustRectIfNeeded(
            RECT& rect,
            const Styles& styles,
            bool isOverlay
        );

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
        );

        void ApplyOverlay(HWND hwnd);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Window State & Position
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void CenterOnScreen(HWND hwnd);
        [[nodiscard]] Size GetScreenSize();
        [[nodiscard]] Size GetWindowSize(HWND hwnd);
        [[nodiscard]] Pos CalculateCenterPosition(const Size& windowSize, const Size& screenSize);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Message & Parameter Extraction
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void ExtractMousePos(LPARAM lParam, int& x, int& y);
        void ExtractSize(LPARAM lParam, int& w, int& h);

    } // namespace WindowUtils
} // namespace Spectrum

#endif // SPECTRUM_CPP_WINDOW_HELPER_H