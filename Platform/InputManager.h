#ifndef SPECTRUM_CPP_INPUT_MANAGER_H
#define SPECTRUM_CPP_INPUT_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Keyboard polling ? InputAction queue.
// Detects key-down transitions per frame via GetAsyncKeyState.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <unordered_map>
#include <vector>

namespace Spectrum::Platform {

    class InputManager final {
    public:
        InputManager() {
            m_keyMap = {
                { VK_SPACE,     InputAction::ToggleCapture         },
                { 'A',          InputAction::ToggleAnimation       },
                { 'S',          InputAction::CycleSpectrumScale    },
                { VK_UP,        InputAction::IncreaseAmplification },
                { VK_DOWN,      InputAction::DecreaseAmplification },
                { VK_LEFT,      InputAction::PrevFFTWindow         },
                { VK_RIGHT,     InputAction::NextFFTWindow         },
                { VK_SUBTRACT,  InputAction::DecreaseBarCount      },
                { VK_OEM_MINUS, InputAction::DecreaseBarCount      },
                { VK_ADD,       InputAction::IncreaseBarCount      },
                { VK_OEM_PLUS,  InputAction::IncreaseBarCount      },
                { 'R',          InputAction::SwitchRenderer        },
                { 'Q',          InputAction::CycleQuality          },
                { 'O',          InputAction::ToggleOverlay         },
                { VK_ESCAPE,    InputAction::Exit                  },
            };
        }

        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        void Update() {
            for (const auto& [key, action] : m_keyMap) {
                const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
                if (down && !m_held[key])
                    m_queue.push_back(action);
                m_held[key] = down;
            }
        }

        [[nodiscard]] std::vector<InputAction> FlushActions() {
            std::vector<InputAction> out;
            out.swap(m_queue);
            return out;
        }

    private:
        std::unordered_map<int, InputAction> m_keyMap;
        std::unordered_map<int, bool>        m_held;
        std::vector<InputAction>             m_queue;
    };

} // namespace Spectrum::Platform

#endif