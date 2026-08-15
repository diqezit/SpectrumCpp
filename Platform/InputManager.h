#ifndef SPECTRUM_CPP_INPUT_MANAGER_H
#define SPECTRUM_CPP_INPUT_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Keyboard polling ? InputAction queue.
// Detects key-down transitions per frame via GetAsyncKeyState.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <vector>

namespace Spectrum::Platform {

    class InputManager final {
    public:
        InputManager() = default;

        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        void Update() {
            for (auto& bind : m_binds)
                if (bind.Pressed())
                    m_queue.push_back(bind.action);
        }

        [[nodiscard]] std::vector<InputAction> FlushActions() {
            std::vector<InputAction> actions;
            actions.swap(m_queue);
            return actions;
        }

    private:
        struct Binding {
            int         key;
            InputAction action;
            bool        held = false;

            bool Pressed() {
                const bool down = GetAsyncKeyState(key) < 0;
                const bool edge = down && !held;
                held = down;
                return edge;
            }
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Bindings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        std::vector<Binding> m_binds{
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

        std::vector<InputAction> m_queue;
    };

} // namespace Spectrum::Platform

#endif