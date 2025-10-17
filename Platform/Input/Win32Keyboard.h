// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the IKeyboard interface for the Win32 platform.
//
// This class uses the GetAsyncKeyState API to poll the current state of a
// keyboard key, providing a concrete implementation for Windows. It is
// intended to be instantiated once and passed to the InputManager.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_WIN32KEYBOARD_H
#define SPECTRUM_CPP_WIN32KEYBOARD_H

#include "IKeyboard.h"
#include "Common/Common.h"

namespace Spectrum::Platform::Input {

    class Win32Keyboard final : public IKeyboard {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsKeyPressed(int virtualKeyCode) const override {
            // 0x8000 is the bit that indicates if the key is currently pressed down
            return (GetAsyncKeyState(virtualKeyCode) & 0x8000) != 0;
        }
    };

} // namespace Spectrum::Platform::Input

#endif // SPECTRUM_CPP_WIN32KEYBOARD_H