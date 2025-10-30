#ifndef SPECTRUM_CPP_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_MESSAGE_HANDLER_H

#include "Common/Common.h"
#include "MessageHandlerBase.h"

namespace Spectrum {
    class ControllerCore;
    class EventBus;

    namespace Platform {
        class WindowManager;

        class MessageHandler final : public MessageHandlerBase {
        public:
            MessageHandler(
                ControllerCore* controller,
                WindowManager* windowManager,
                EventBus* bus
            );
            ~MessageHandler() noexcept override = default;

            MessageHandler(const MessageHandler&) = delete;
            MessageHandler& operator=(const MessageHandler&) = delete;
            MessageHandler(MessageHandler&&) = delete;
            MessageHandler& operator=(MessageHandler&&) = delete;

            LRESULT HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

        private:
            [[nodiscard]] LRESULT HandleLifecycleMessage(UINT msg);
            [[nodiscard]] LRESULT HandleSpecialMessage(UINT msg);

            void SubscribeToEvents(EventBus* bus);

            ControllerCore* m_controller;
            WindowManager* m_windowManager;
        };

    }
}

#endif