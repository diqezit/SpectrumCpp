#ifndef SPECTRUM_CPP_UI_MESSAGE_HANDLER_H
#define SPECTRUM_CPP_UI_MESSAGE_HANDLER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UI window message handler — ImGui + borderless drag.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Platform/MessageHandlerBase.h"
#include "Platform/WindowManager.h"
#include "UI/UIManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    class UIMessageHandler final : public MessageHandlerBase {
    public:
        UIMessageHandler(
            ControllerCore*,
            WindowManager* wm,
            UIManager* ui,
            EventBus*)
            : m_wm(wm)
            , m_ui(ui)
        {
            if (!wm || !ui)
                throw std::invalid_argument(
                    "UIMessageHandler: null dependency");
        }

        LRESULT HandleWindowMessage(
            HWND hwnd, UINT msg,
            WPARAM wp, LPARAM lp) override
        {
            // ImGui gets first chance
            if (m_ui->HandleMessage(hwnd, msg, wp, lp))
                return 0;

            switch (msg) {
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Paint
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_PAINT: {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
                return 0;
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_ENTERSIZEMOVE:
            case WM_EXITSIZEMOVE:
            case WM_SIZE:
                return DispatchResize(hwnd, msg, wp, lp,
                    [this] {
                        m_wm->OnUIResizeStart();
                    },
                    [this](HWND h) {
                        m_wm->OnUIResizeEnd(h);
                    },
                    [this](HWND h, int w, int ht) {
                        m_wm->OnUIResize(h, w, ht);
                    });

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Borderless drag
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_NCHITTEST: {
                POINT pt = {
                    GET_X_LPARAM(lp),
                    GET_Y_LPARAM(lp)
                };
                ScreenToClient(hwnd, &pt);

                if (pt.y < kTitleBarHeight
                    && !ImGui::IsAnyItemActive()
                    && !ImGui::IsAnyItemHovered())
                    return HTCAPTION;

                return HTCLIENT;
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Close — hide, don't destroy
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            case WM_CLOSE:
                ::ShowWindow(hwnd, SW_HIDE);
                return 0;

            case WM_DESTROY:
                return 0;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Mouse + erase + default
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            default:
                return HandleCommon(hwnd, msg, wp, lp);
            }
        }

    private:
        static constexpr int kTitleBarHeight = 40;

        WindowManager* m_wm;
        UIManager*     m_ui;
    };

} // namespace Spectrum::Platform

#endif