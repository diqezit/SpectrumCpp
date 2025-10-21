// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the InputManager, responsible for capturing keyboard input and
// translating it into actionable commands for the application.
//
// The InputManager implements a data-driven input system that polls keyboard
// state each frame, detects key press events (transitions from up to down),
// and maps them to application-specific InputAction commands. It maintains
// a queue of actions that occurred during the current frame, which can be
// consumed by the main application loop.
//
// Key Responsibilities:
// - Poll keyboard state using platform-specific APIs
// - Detect key press events (edge detection: not pressed -> pressed)
// - Map virtual key codes to InputAction commands
// - Maintain an action queue for the current frame
// - Support data-driven key binding configuration
//
// Design Philosophy:
// - Stateful: tracks previous frame's key states for edge detection
// - Data-driven: key mappings configured in constructor, easily extensible
// - Non-blocking: polling-based approach, no message queue dependencies
// - Frame-based: actions are queued and consumed per frame
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_INPUT_MANAGER_H
#define SPECTRUM_CPP_INPUT_MANAGER_H

#include "Common.h"
#include <unordered_map>
#include <vector>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // InputManager Class
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class InputManager final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        InputManager();
        ~InputManager() noexcept = default;

        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;
        InputManager(InputManager&&) = delete;
        InputManager& operator=(InputManager&&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<InputAction> GetActions();

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void InitializeKeyMappings();
        void PollKeys();
        void ProcessSingleKey(int key, InputAction action);
        [[nodiscard]] bool IsKeyCurrentlyPressed(int key) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        std::unordered_map<int, bool> m_keyStates;
        std::unordered_map<int, InputAction> m_keyMappings;
        std::vector<InputAction> m_actionQueue;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_INPUT_MANAGER_H