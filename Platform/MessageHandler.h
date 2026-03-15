#ifndef SPECTRUM_CPP_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_MESSAGE_HANDLER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Main window message handler.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Platform/MessageHandlerBase.h"
#include "App/ControllerCore.h"
#include "Common/EventBus.h"
#include "Platform/WindowManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    class MessageHandler final : public MessageHandlerBase {
    public:
        MessageHandler(
            ControllerCore* ctrl,
            WindowManager* wm,
            EventBus* bus)
            : m_ctrl(ctrl)
            , m_wm(wm)
        {
            if (!ctrl)
                throw std::invalid_argument(
                    "MessageHandler: null controller");
            if (!wm)
                throw std::invalid_argument(
                    "MessageHandler: null WindowManager");

            if (bus) {
                bus->Subscribe(InputAction::ToggleOverlay,
                    [this] { m_wm->ToggleOverlay(); });

                bus->Subscribe(InputAction::Exit, [this] {
                    m_wm->IsOverlayMode()
                        ? m_wm->ToggleOverlay()
                        : m_ctrl->OnCloseRequest();
                    });
            }
        }

        LRESULT HandleWindowMessage(
            HWND hwnd, UINT msg,
            WPARAM wp, LPARAM lp) override
        {
            switch (msg) {
                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
                // Lifecycle
                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_CLOSE:
                m_wm->IsOverlayMode()
                    ? m_wm->ToggleOverlay()
                    : m_ctrl->OnCloseRequest();
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
                // Resize
                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_ENTERSIZEMOVE:
            case WM_EXITSIZEMOVE:
            case WM_SIZE:
                return DispatchResize(hwnd, msg, wp, lp,
                    [this] { m_wm->OnResizeStart(); },
                    [this](HWND h) { m_wm->OnResizeEnd(h); },
                    [this](HWND h, int w, int ht) {
                        m_wm->OnResize(h, w, ht);
                    });

                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
                // Hit test
                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_NCHITTEST:
                return m_wm->IsOverlayMode()
                    ? HTCAPTION : HTCLIENT;

                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
                // Mouse + erase + default
                // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            default:
                return HandleCommon(hwnd, msg, wp, lp,
                    [this] {
                        m_ctrl->OnMainWindowClick(
                            m_mouse.position);
                    });
            }
        }

    private:
        ControllerCore* m_ctrl;
        WindowManager* m_wm;
    };

} // namespace Spectrum::Platform

#endif