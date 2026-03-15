#ifndef SPECTRUM_CPP_MESSAGE_HANDLER_BASE_H
#define SPECTRUM_CPP_MESSAGE_HANDLER_BASE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Abstract base for Win32 message handlers.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <functional>

namespace Spectrum::Platform {

    struct MouseState {
        Point position{};
        bool  leftButtonDown = false;
        bool  rightButtonDown = false;
        bool  middleButtonDown = false;
        float wheelDelta = 0.0f;
    };

    class MessageHandlerBase {
    public:
        virtual ~MessageHandlerBase() noexcept = default;

        MessageHandlerBase(const MessageHandlerBase&) = delete;
        MessageHandlerBase& operator=(const MessageHandlerBase&) = delete;

        [[nodiscard]] const MouseState& GetMouseState() const noexcept {
            return m_mouse;
        }

        virtual LRESULT HandleWindowMessage(
            HWND, UINT, WPARAM, LPARAM) = 0;

    protected:
        MessageHandlerBase() = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Common — mouse + erase + default
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        LRESULT HandleCommon(
            HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
            const std::function<void()>& onLeftDown = nullptr)
        {
            switch (msg) {
            case WM_ERASEBKGND:
                return 1;

            case WM_MOUSEMOVE:
                m_mouse.position = {
                    static_cast<float>(GET_X_LPARAM(lp)),
                    static_cast<float>(GET_Y_LPARAM(lp))
                };
                return 0;

            case WM_MOUSEWHEEL:
                m_mouse.wheelDelta =
                    static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp))
                    / WHEEL_DELTA;
                return 0;

            case WM_LBUTTONDOWN:
                SetButton(msg, true, hwnd);
                if (onLeftDown) onLeftDown();
                return 0;

            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                SetButton(msg, true, hwnd);
                return 0;

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
                SetButton(msg, false, hwnd);
                return 0;

            default:
                return DefWindowProc(hwnd, msg, wp, lp);
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Resize dispatch
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        LRESULT DispatchResize(
            HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
            const std::function<void()>& onStart,
            const std::function<void(HWND)>& onEnd,
            const std::function<void(HWND, int, int)>& onSize)
        {
            switch (msg) {
            case WM_ENTERSIZEMOVE:
                if (onStart) onStart();
                return 0;

            case WM_EXITSIZEMOVE:
                if (onEnd) onEnd(hwnd);
                return 0;

            case WM_SIZE:
                if (wp != SIZE_MINIMIZED && onSize)
                    onSize(hwnd, LOWORD(lp), HIWORD(lp));
                return 0;

            default:
                return DefWindowProc(hwnd, msg, wp, lp);
            }
        }

        MouseState m_mouse{};

    private:
        void SetButton(UINT msg, bool down, HWND hwnd) {
            if (down) SetCapture(hwnd);
            else      ReleaseCapture();

            switch (msg) {
            case WM_LBUTTONDOWN: case WM_LBUTTONUP:
                m_mouse.leftButtonDown = down;
                break;
            case WM_RBUTTONDOWN: case WM_RBUTTONUP:
                m_mouse.rightButtonDown = down;
                break;
            case WM_MBUTTONDOWN: case WM_MBUTTONUP:
                m_mouse.middleButtonDown = down;
                break;
            }
        }
    };

} // namespace Spectrum::Platform

#endif