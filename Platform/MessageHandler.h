// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the MessageHandler, responsible for processing raw Win32 window
// messages and translating them into application-level events.
//
// This class decouples the WindowManager from the intricacies of the Win32
// message loop. It maintains input state (e.g., mouse) and delegates
// high-level actions to the ControllerCore and other managers, adhering to
// the Single Responsibility Principle.
//
// Design notes:
// - All business logic moved to .cpp file
// - Message handling split by category (lifecycle, resize, mouse, special)
// - Dependency validation extracted to separate methods
// - Resize debouncing encapsulated
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
            // Dependency Validation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void ValidateDependencies() const;
            void ValidateDependency(void* ptr, const char* name) const;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Message Category Handlers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] LRESULT HandleLifecycleMessage(UINT msg);
            [[nodiscard]] LRESULT HandleResizeMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            [[nodiscard]] LRESULT HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);
            [[nodiscard]] LRESULT HandleSpecialMessage(UINT msg);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Lifecycle Message Handlers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void HandleCloseRequest();
            void HandleDestroyRequest();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Resize Message Handlers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void HandleResizeStart();
            void HandleResizeEnd(HWND hwnd);
            void HandleResize(HWND hwnd, WPARAM wParam, LPARAM lParam);

            [[nodiscard]] bool IsMinimized(WPARAM wParam) const;
            [[nodiscard]] bool ShouldProcessResize(int width, int height);
            void ExtractResizeDimensions(LPARAM lParam, int& outWidth, int& outHeight) const;
            void UpdateResizeCache(int width, int height);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Mouse Message Handlers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void HandleMouseMove(LPARAM lParam);
            void HandleLeftButtonDown();
            void HandleLeftButtonUp();
            void HandleRightButtonDown();
            void HandleRightButtonUp();
            void HandleMouseWheel(WPARAM wParam);

            void UpdateMousePosition(LPARAM lParam);
            void SetLeftButtonState(bool pressed);
            void SetRightButtonState(bool pressed);
            void UpdateWheelDelta(WPARAM wParam);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Special Message Handlers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] LRESULT HandleHitTest();
            [[nodiscard]] LRESULT HandleEraseBackground();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Event Subscription
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void SubscribeToEvents(EventBus* bus);
            void SubscribeToOverlayToggle(EventBus* bus);
            void SubscribeToExitAction(EventBus* bus);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Exit Handling
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void OnExitRequest();
            void HandleOverlayExit();
            void HandleNormalExit();

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Member Variables
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            ControllerCore* m_controller;
            WindowManager* m_windowManager;
            UIManager* m_uiManager;
            MouseState m_mouseState{};

            // Resize debouncing
            int m_lastResizeWidth = 0;
            int m_lastResizeHeight = 0;
        };
    } // namespace Platform
} // namespace Spectrum

#endif // SPECTRUM_CPP_MESSAGE_HANDLER_H