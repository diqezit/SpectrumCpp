// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the InputManager, which polls keyboard state and maps key
// presses to application-specific actions.
//
// This implementation provides frame-based keyboard input handling through
// polling. It uses an IKeyboard dependency to abstract the platform-specific
// key state checks, enabling testability and adhering to SRP.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "InputManager.h"
#include <stdexcept>

namespace Spectrum::Platform::Input {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    InputManager::InputManager(std::unique_ptr<IKeyboard> keyboard) :
        m_keyboard(std::move(keyboard))
    {
        if (!m_keyboard) throw std::invalid_argument("keyboard dependency cannot be null");
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
        {
            ProcessSingleKey(key, action);
        }
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
        return m_keyboard->IsKeyPressed(key);
    }

} // namespace Spectrum::Platform::Input