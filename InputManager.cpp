// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the InputManager, which polls keyboard state and maps key
// presses to application-specific actions.
//
// This implementation provides frame-based keyboard input handling through
// polling. Each Update() call checks the state of all registered keys,
// detects press events (state transitions from up to down), and queues
// corresponding actions. The action queue is consumed by GetActions(),
// which returns all actions that occurred during the current frame.
//
// Key Implementation Details:
// - Edge detection: only triggers on transition from not-pressed to pressed
// - Data-driven mapping: key bindings initialized in constructor
// - Structured bindings for clean iteration over mappings
// - Move semantics for efficient action queue transfer
// - Platform abstraction via Utils::IsKeyPressed()
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "InputManager.h"
#include "PlatformUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    InputManager::InputManager()
    {
        InitializeKeyMappings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void InputManager::Update()
    {
        PollKeys();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::vector<InputAction> InputManager::GetActions()
    {
        std::vector<InputAction> actions = std::move(m_actionQueue);
        m_actionQueue.clear();
        return actions;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void InputManager::InitializeKeyMappings()
    {
        m_keyMappings[VK_SPACE] = InputAction::ToggleCapture;
        m_keyMappings['A'] = InputAction::ToggleAnimation;
        m_keyMappings['S'] = InputAction::CycleSpectrumScale;
        m_keyMappings[VK_UP] = InputAction::IncreaseAmplification;
        m_keyMappings[VK_DOWN] = InputAction::DecreaseAmplification;
        m_keyMappings[VK_LEFT] = InputAction::PrevFFTWindow;
        m_keyMappings[VK_RIGHT] = InputAction::NextFFTWindow;
        m_keyMappings[VK_SUBTRACT] = InputAction::DecreaseBarCount;
        m_keyMappings[VK_OEM_MINUS] = InputAction::DecreaseBarCount;
        m_keyMappings[VK_ADD] = InputAction::IncreaseBarCount;
        m_keyMappings[VK_OEM_PLUS] = InputAction::IncreaseBarCount;
        m_keyMappings['R'] = InputAction::SwitchRenderer;
        m_keyMappings['Q'] = InputAction::CycleQuality;
        m_keyMappings['O'] = InputAction::ToggleOverlay;
        m_keyMappings[VK_ESCAPE] = InputAction::Exit;
    }

    void InputManager::PollKeys()
    {
        for (const auto& [key, action] : m_keyMappings)
            ProcessSingleKey(key, action);
    }

    void InputManager::ProcessSingleKey(int key, InputAction action)
    {
        const bool isCurrentlyPressed = IsKeyCurrentlyPressed(key);
        const bool wasPreviouslyPressed = m_keyStates[key];

        if (isCurrentlyPressed && !wasPreviouslyPressed)
        {
            m_keyStates[key] = true;
            m_actionQueue.push_back(action);
        }
        else if (!isCurrentlyPressed)
        {
            m_keyStates[key] = false;
        }
    }

    [[nodiscard]] bool InputManager::IsKeyCurrentlyPressed(int key) const
    {
        return Utils::IsKeyPressed(key);
    }

} // namespace Spectrum