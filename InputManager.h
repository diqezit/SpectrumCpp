// InputManager.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// InputManager.h: Handles and dispatches user input events.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_INPUT_MANAGER_H
#define SPECTRUM_CPP_INPUT_MANAGER_H

#include "Common.h"

namespace Spectrum {

    class ControllerCore;

    class InputManager {
    public:
        explicit InputManager(ControllerCore& controller);

        void OnKeyPress(int key);
        void OnMouseMove(int x, int y);
        void OnMouseClick(int x, int y);

    private:
        ControllerCore& m_controller;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_INPUT_MANAGER_H