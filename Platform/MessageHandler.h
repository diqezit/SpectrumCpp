// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the MessageHandler, responsible for processing raw Win32 window
// messages and translating them into application-level events.
//
// This class decouples the WindowManager from the intricacies of the Win32
// message loop. It maintains input state (e.g., mouse) and delegates
// high-level actions to the ControllerCore and other managers, adhering to
// the Single Responsibility Principle.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_MESSAGE_HANDLER_H

#include "Common/Common.h"

namespace Spectrum {
    class ControllerCore;
    class EventBus;
    class UIManager;

    namespace Platform {
        class WindowManager;

        class MessageHandler final {
        public:
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Public Structures
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            struct MouseState {
                Point position{ 0.0f, 0.0f };
                bool leftButtonDown = false;
                bool rightButtonDown = false;
                bool middleButtonDown = false;
                float wheelDelta = 0.0f;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Lifecycle Management
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            MessageHandler(ControllerCore* controller, WindowManager* windowManager, UIManager* uiManager, EventBus* bus);
            ~MessageHandler() noexcept = default;

            MessageHandler(const MessageHandler&) = delete;
            MessageHandler& operator=(const MessageHandler&) = delete;
            MessageHandler(MessageHandler&&) = delete;
            MessageHandler& operator=(MessageHandler&&) = delete;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Window Message Handling
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            LRESULT HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Public Getters
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] const MouseState& GetMouseState() const noexcept;

        private:
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Event Handling
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void SubscribeToEvents(EventBus* bus);
            void OnExitRequest();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Member Variables
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            ControllerCore* m_controller;
            WindowManager* m_windowManager;
            UIManager* m_uiManager;
            MouseState m_mouseState{};
        };
    } // namespace Platform
} // namespace Spectrum

#endif // SPECTRUM_CPP_MESSAGE_HANDLER_H