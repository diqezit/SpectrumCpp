// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Defines the InputManager, responsible for capturing keyboard input and
// translating it into a queue of actionable commands for the application.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_INPUT_MANAGER_H
#define SPECTRUM_CPP_INPUT_MANAGER_H

#include "Common.h"
#include <map>
#include <vector>

namespace Spectrum {

    class InputManager {
    public:
        InputManager();

        void Update();

        std::vector<InputAction> GetActions();

    private:
        void InitializeKeyMappings();
        void PollKeys();
        void ProcessSingleKey(int key, InputAction action);

        std::map<int, bool> m_keyStates;
        std::map<int, InputAction> m_keyMappings;
        std::vector<InputAction> m_actionQueue;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_INPUT_MANAGER_H