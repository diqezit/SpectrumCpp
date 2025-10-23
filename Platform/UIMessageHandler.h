// UIMessageHandler.h
#ifndef SPECTRUM_CPP_UI_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_UI_MESSAGE_HANDLER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the UIMessageHandler for processing UI window messages.
//
// This class is dedicated to the UI window, processing raw Win32 messages,
// updating mouse state, and handling window events like resizing or closing.
// It ensures that UI-specific actions (like hiding the window instead of
// closing the app) are handled correctly.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"

namespace Spectrum {

    // Forward declarations
    class ControllerCore;
    class EventBus;
    class UIManager;

    namespace Platform {

        class WindowManager;

        // State of the mouse specific to the UI window.
        struct UIMouseState
        {
            Point position{ 0.0f, 0.0f };
            bool leftButtonDown = false;
            bool rightButtonDown = false;
            bool middleButtonDown = false;
            float wheelDelta = 0.0f;
        };

        class UIMessageHandler final
        {
        public:
            // --- Lifecycle & Initialization ---
            explicit UIMessageHandler(
                ControllerCore* controller,
                WindowManager* windowManager,
                UIManager* uiManager,
                EventBus* bus
            );
            ~UIMessageHandler() noexcept = default;

            // Rule of Five: This class manages no resources directly and is not meant to be copied/moved.
            UIMessageHandler(const UIMessageHandler&) = delete;
            UIMessageHandler& operator=(const UIMessageHandler&) = delete;
            UIMessageHandler(UIMessageHandler&&) = delete;
            UIMessageHandler& operator=(UIMessageHandler&&) = delete;

            // --- Message Processing ---
            [[nodiscard]] LRESULT HandleWindowMessage(
                HWND hwnd,
                UINT msg,
                WPARAM wParam,
                LPARAM lParam
            );

            // --- State Queries ---
            [[nodiscard]] const UIMouseState& GetMouseState() const noexcept;

        private:
            // --- Message Handlers ---
            void HandleResize(HWND hwnd, WPARAM wParam, LPARAM lParam);
            void HandleMouseMove(LPARAM lParam);
            void HandleMouseDown(UINT msg, HWND hwnd);
            void HandleMouseUp(UINT msg);
            void HandleMouseWheel(WPARAM wParam);
            void HandleClose(HWND hwnd);

            // --- Dependencies (Non-owning) ---
            ControllerCore* m_controller;
            WindowManager* m_windowManager;
            UIManager* m_uiManager;
            EventBus* m_bus;

            // --- State ---
            UIMouseState m_mouseState;
        };

    } // namespace Platform
} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_MESSAGE_HANDLER_H