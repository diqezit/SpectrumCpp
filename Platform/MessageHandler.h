#ifndef SPECTRUM_CPP_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_MESSAGE_HANDLER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the MessageHandler, responsible for processing raw Win32 window
// messages and translating them into application-level events.
//
// This class decouples the WindowManager from the intricacies of the Win32
// message loop. It maintains input state (e.g., mouse) and delegates
// high-level actions to the ControllerCore and other managers.
//
// Design notes:
// - Simplified message handling structure
// - Direct processing without excessive abstraction
// - Message handling split by category (lifecycle, resize, mouse, special)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"

namespace Spectrum {
    class ControllerCore;
    class EventBus;
    class UIManager;

    namespace Platform {
        class WindowManager;

        class MessageHandler final
        {
        public:
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Public Structures
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            struct MouseState
            {
                Point position{ 0.0f, 0.0f };
                bool leftButtonDown = false;
                bool rightButtonDown = false;
                bool middleButtonDown = false;
                float wheelDelta = 0.0f;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Lifecycle Management
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            MessageHandler(
                ControllerCore* controller,
                WindowManager* windowManager,
                UIManager* uiManager,
                EventBus* bus
            );
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
            // Message Category Handlers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] LRESULT HandleLifecycleMessage(UINT msg);
            [[nodiscard]] LRESULT HandleResizeMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            [[nodiscard]] LRESULT HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);
            [[nodiscard]] LRESULT HandleSpecialMessage(UINT msg);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Event Subscription
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void SubscribeToEvents(EventBus* bus);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Member Variables
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            ControllerCore* m_controller;
            WindowManager* m_windowManager;
            UIManager* m_uiManager;
            MouseState m_mouseState{};

            int m_lastResizeWidth = 0;
            int m_lastResizeHeight = 0;
        };

    } // namespace Platform
} // namespace Spectrum

#endif // SPECTRUM_CPP_MESSAGE_HANDLER_H