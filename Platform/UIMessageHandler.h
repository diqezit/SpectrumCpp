#ifndef SPECTRUM_CPP_UI_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_UI_MESSAGE_HANDLER_H

#include "Common/Common.h"
#include "MessageHandlerBase.h"

namespace Spectrum {
    class ControllerCore;
    class EventBus;
    class UIManager;

    namespace Platform {
        class WindowManager;

        class UIMessageHandler final : public MessageHandlerBase {
        public:
            explicit UIMessageHandler(
                ControllerCore* controller,
                WindowManager* windowManager,
                UIManager* uiManager,
                EventBus* bus
            );
            ~UIMessageHandler() noexcept override = default;

            UIMessageHandler(const UIMessageHandler&) = delete;
            UIMessageHandler& operator=(const UIMessageHandler&) = delete;
            UIMessageHandler(UIMessageHandler&&) = delete;
            UIMessageHandler& operator=(UIMessageHandler&&) = delete;

            [[nodiscard]] LRESULT HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

        private:
            void HandleClose(HWND hwnd);

            ControllerCore* m_controller;
            WindowManager* m_windowManager;
            UIManager* m_uiManager;
            EventBus* m_bus;
        };

    }
}

#endif