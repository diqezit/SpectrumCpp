#ifndef SPECTRUM_CPP_PLATFORM_UTILS_H
#define SPECTRUM_CPP_PLATFORM_UTILS_H

#include "Common.h"

namespace Spectrum {
    namespace Utils {

        [[nodiscard]] inline Point GetMousePosition(LPARAM lParam) noexcept {
            return Point{
                static_cast<float>(GET_X_LPARAM(lParam)),
                static_cast<float>(GET_Y_LPARAM(lParam))
            };
        }

        [[nodiscard]] inline bool IsKeyPressed(int vkCode) noexcept {
            return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        }

    } // namespace Utils
} // namespace Spectrum

#endif