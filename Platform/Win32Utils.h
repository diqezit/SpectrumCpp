// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// A collection of stateless helper functions for common Win32 API
// operations and data extraction.
//
// Defines the Win32Utils namespace, a library of pure, header-only inline
// functions that abstract low-level Win32 API details for window style
// creation, geometry calculations, and message parameter extraction.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WIN32_UTILS_H
#define SPECTRUM_CPP_WIN32_UTILS_H

#include "Common/Common.h"

namespace Spectrum::Platform::Win32Utils {

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

    [[nodiscard]] inline Styles MakeStyles(bool isOverlay) {
        return isOverlay
            ? Styles{ WS_POPUP, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW }
        : Styles{ WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW };
    }

    inline void AdjustRectForStyles(RECT& rect, const Styles& styles) {
        AdjustWindowRectEx(&rect, styles.style, FALSE, styles.exStyle);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Geometry Calculations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] inline Size GetScreenSize() {
        return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    }

    [[nodiscard]] inline Pos CalculateCenterPosition(const Size& windowSize, const Size& screenSize) {
        return {
            (screenSize.w - windowSize.w) / 2,
            (screenSize.h - windowSize.h) / 2
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Message & Parameter Extraction
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline void ExtractMousePos(LPARAM lParam, int& x, int& y) {
        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);
    }

    inline void ExtractSize(LPARAM lParam, int& w, int& h) {
        w = LOWORD(lParam);
        h = HIWORD(lParam);
    }

} // namespace Spectrum::Platform::Win32Utils

#endif // SPECTRUM_CPP_WIN32_UTILS_H