// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Implements the InputManager, which polls keyboard state and maps key
// presses to application-specific actions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "InputManager.h"
#include "PlatformUtils.h"

namespace Spectrum {

    InputManager::InputManager() {
        InitializeKeyMappings();
    }

    void InputManager::InitializeKeyMappings() {
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

    void InputManager::Update() {
        PollKeys();
    }

    std::vector<InputAction> InputManager::GetActions() {
        if (m_actionQueue.empty()) return {};

        std::vector<InputAction> actions = std::move(m_actionQueue);
        m_actionQueue.clear();
        return actions;
    }

    void InputManager::PollKeys() {
        for (const auto& [key, action] : m_keyMappings) {
            ProcessSingleKey(key, action);
        }
    }

    void InputManager::ProcessSingleKey(int key, InputAction action) {
        bool isPressed = Utils::IsKeyPressed(key);

        if (isPressed && !m_keyStates[key]) {
            m_keyStates[key] = true;
            m_actionQueue.push_back(action);
        }
        else if (!isPressed) {
            m_keyStates[key] = false;
        }
    }

} // namespace Spectrum