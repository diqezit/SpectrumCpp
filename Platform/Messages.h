#ifndef SPECTRUM_CPP_MESSAGES_H
#define SPECTRUM_CPP_MESSAGES_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Win32 message handlers — mouse, resize, main window, UI drag.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/EventBus.h"
#include "UI/UI.h"

namespace Spectrum {

    class Core;

    namespace Platform {

        class WindowManager;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Mouse
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct MouseState {
            Point position{};
            bool  leftButtonDown = false;
            bool  rightButtonDown = false;
            bool  middleButtonDown = false;
            float wheelDelta = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Base
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        class MessageHandlerBase {
        public:
            virtual ~MessageHandlerBase() noexcept = default;

            MessageHandlerBase(const MessageHandlerBase&) = delete;
            MessageHandlerBase& operator=(const MessageHandlerBase&) = delete;

            [[nodiscard]] const MouseState& GetMouseState() const noexcept { return m_mouse; }

            virtual LRESULT HandleWindowMessage(HWND, UINT, WPARAM, LPARAM) = 0;

        protected:
            MessageHandlerBase() = default;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Common — mouse + erase + default
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            template<typename OnClick = void(*)()>
            LRESULT HandleCommon(
                HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                OnClick&& onLeftDown = +[] {})
            {
                switch (msg) {
                case WM_ERASEBKGND:  return 1;
                case WM_MOUSEMOVE:   return TrackMove(lp);
                case WM_MOUSEWHEEL:  return TrackWheel(wp);
                case WM_LBUTTONDOWN: return Press(m_mouse.leftButtonDown, hwnd, onLeftDown);
                case WM_RBUTTONDOWN: return Press(m_mouse.rightButtonDown, hwnd);
                case WM_MBUTTONDOWN: return Press(m_mouse.middleButtonDown, hwnd);
                case WM_LBUTTONUP:   return Release(m_mouse.leftButtonDown);
                case WM_RBUTTONUP:   return Release(m_mouse.rightButtonDown);
                case WM_MBUTTONUP:   return Release(m_mouse.middleButtonDown);
                default:             return DefWindowProc(hwnd, msg, wp, lp);
                }
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            template<typename OnStart, typename OnEnd, typename OnSize>
            LRESULT DispatchResize(
                HWND hwnd, UINT msg, LPARAM lp,
                OnStart&& onStart, OnEnd&& onEnd, OnSize&& onSize)
            {
                switch (msg) {
                case WM_ENTERSIZEMOVE: onStart(); break;
                case WM_EXITSIZEMOVE:  onEnd(hwnd); break;
                case WM_SIZE:          onSize(hwnd, LOWORD(lp), HIWORD(lp)); break;
                default:               return DefWindowProc(hwnd, msg, 0, lp);
                }
                return 0;
            }

            MouseState m_mouse{};

        private:
            LRESULT TrackMove(LPARAM lp) {
                m_mouse.position = { float(GET_X_LPARAM(lp)), float(GET_Y_LPARAM(lp)) };
                return 0;
            }

            LRESULT TrackWheel(WPARAM wp) {
                m_mouse.wheelDelta = float(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA;
                return 0;
            }

            template<typename Extra = void(*)()>
            LRESULT Press(bool& button, HWND hwnd, Extra&& extra = +[] {}) {
                SetCapture(hwnd);
                button = true;
                extra();
                return 0;
            }

            LRESULT Release(bool& button) {
                ReleaseCapture();
                button = false;
                return 0;
            }
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Main window
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        class MessageHandler final : public MessageHandlerBase {
        public:
            MessageHandler(Core* ctrl, WindowManager* wm, EventBus* bus);
            LRESULT HandleWindowMessage(HWND, UINT, WPARAM, LPARAM) override;

        private:
            LRESULT HandleClose();
            LRESULT HandleResize(HWND hwnd, UINT msg, LPARAM lp);

            Core* m_ctrl;
            WindowManager* m_wm;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // UI window
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        class UIMessageHandler final : public MessageHandlerBase {
        public:
            UIMessageHandler(Core*, WindowManager* wm, UIManager* ui, EventBus*)
                : m_wm(wm), m_ui(ui) {
            }

            LRESULT HandleWindowMessage(HWND, UINT, WPARAM, LPARAM) override;

        private:
            static constexpr int kTitleBarHeight = 40;

            static LRESULT HandlePaint(HWND hwnd) {
                ValidateRect(hwnd, nullptr);
                return 0;
            }

            static LRESULT HandleClose(HWND hwnd) {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }

            static LRESULT HitTest(HWND hwnd, LPARAM lp) {
                POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                ScreenToClient(hwnd, &pt);

                const bool title = pt.y < kTitleBarHeight;
                const bool idle = !ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered();
                return (title && idle) ? HTCAPTION : HTCLIENT;
            }

            LRESULT HandleResize(HWND hwnd, UINT msg, LPARAM lp);

            WindowManager* m_wm;
            UIManager* m_ui;
        };

    } // namespace Platform
} // namespace Spectrum

#endif

#if defined(SPECTRUM_CPP_CORE_READY) && !defined(SPECTRUM_CPP_MESSAGES_IMPL)
#define SPECTRUM_CPP_MESSAGES_IMPL

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main window
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline MessageHandler::MessageHandler(Core* ctrl, WindowManager* wm, EventBus* bus)
        : m_ctrl(ctrl), m_wm(wm) {
        bus->Subscribe(InputAction::ToggleOverlay, [this] { m_wm->ToggleOverlay(); });
        bus->Subscribe(InputAction::Exit, [this] { HandleClose(); });
    }

    inline LRESULT MessageHandler::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Lifecycle
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_CLOSE:   return HandleClose();
        case WM_DESTROY: PostQuitMessage(0); return 0;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SIZE:
            return HandleResize(hwnd, msg, lp);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Hit test
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_NCHITTEST:
            return m_wm->IsOverlayMode() ? HTCAPTION : HTCLIENT;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Mouse + erase + default
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        default:
            return HandleCommon(hwnd, msg, wp, lp,
                [this] { m_ctrl->OnMainWindowClick(m_mouse.position); });
        }
    }

    inline LRESULT MessageHandler::HandleClose() {
        m_wm->IsOverlayMode()
            ? m_wm->ToggleOverlay()
            : m_ctrl->OnCloseRequest();
        return 0;
    }

    inline LRESULT MessageHandler::HandleResize(HWND hwnd, UINT msg, LPARAM lp) {
        return DispatchResize(hwnd, msg, lp,
            [this] { m_wm->OnResizeStart(); },
            [this](HWND h) { m_wm->OnResizeEnd(h); },
            [this](HWND h, int w, int ht) { m_wm->OnResize(h, w, ht); });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // UI window
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline LRESULT UIMessageHandler::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (m_ui->HandleMessage(hwnd, msg, wp, lp))
            return 0;

        switch (msg) {
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Paint
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_PAINT:
            return HandlePaint(hwnd);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SIZE:
            return HandleResize(hwnd, msg, lp);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Borderless drag
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_NCHITTEST:
            return HitTest(hwnd, lp);

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Close — hide, don't destroy
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        case WM_CLOSE:
            return HandleClose(hwnd);

        case WM_DESTROY:
            return 0;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Mouse + erase + default
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        default:
            return HandleCommon(hwnd, msg, wp, lp);
        }
    }

    inline LRESULT UIMessageHandler::HandleResize(HWND hwnd, UINT msg, LPARAM lp) {
        return DispatchResize(hwnd, msg, lp,
            [this] { m_wm->OnUIResizeStart(); },
            [this](HWND h) { m_wm->OnUIResizeEnd(h); },
            [this](HWND h, int w, int ht) { m_wm->OnUIResize(h, w, ht); });
    }

} // namespace Spectrum::Platform

#endif