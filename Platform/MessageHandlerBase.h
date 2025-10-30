#ifndef SPECTRUM_CPP_MESSAGE_HANDLER_BASE_H
#define SPECTRUM_CPP_MESSAGE_HANDLER_BASE_H

#include "Common/Common.h"
#include <functional>

namespace Spectrum::Platform {

    struct MouseState {
        Point position{ 0.0f, 0.0f };
        bool leftButtonDown = false;
        bool rightButtonDown = false;
        bool middleButtonDown = false;
        float wheelDelta = 0.0f;
    };

    class MessageHandlerBase {
    public:
        virtual ~MessageHandlerBase() noexcept = default;

        MessageHandlerBase(const MessageHandlerBase&) = delete;
        MessageHandlerBase& operator=(const MessageHandlerBase&) = delete;
        MessageHandlerBase(MessageHandlerBase&&) = delete;
        MessageHandlerBase& operator=(MessageHandlerBase&&) = delete;

        [[nodiscard]] const MouseState& GetMouseState() const noexcept {
            return m_mouseState;
        }

        virtual LRESULT HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

    protected:
        MessageHandlerBase() = default;

        void HandleMouseMoveBase(LPARAM lParam) {
            m_mouseState.position.x = static_cast<float>(GET_X_LPARAM(lParam));
            m_mouseState.position.y = static_cast<float>(GET_Y_LPARAM(lParam));
        }

        void HandleMouseWheelBase(WPARAM wParam) {
            m_mouseState.wheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        }

        void HandleMouseButtonDownBase(UINT msg, HWND hwnd) {
            if (hwnd) {
                SetCapture(hwnd);
            }

            switch (msg) {
            case WM_LBUTTONDOWN: m_mouseState.leftButtonDown = true; break;
            case WM_RBUTTONDOWN: m_mouseState.rightButtonDown = true; break;
            case WM_MBUTTONDOWN: m_mouseState.middleButtonDown = true; break;
            }
        }

        void HandleMouseButtonUpBase(UINT msg) {
            ReleaseCapture();

            switch (msg) {
            case WM_LBUTTONUP: m_mouseState.leftButtonDown = false; break;
            case WM_RBUTTONUP: m_mouseState.rightButtonDown = false; break;
            case WM_MBUTTONUP: m_mouseState.middleButtonDown = false; break;
            }
        }

        LRESULT ProcessResizeMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            const std::function<void()>& onStart,
            const std::function<void(HWND)>& onEnd,
            const std::function<void(HWND, int, int)>& onSize) {
            switch (msg) {
            case WM_ENTERSIZEMOVE:
                if (onStart) onStart();
                return 0;

            case WM_EXITSIZEMOVE:
                if (onEnd) onEnd(hwnd);
                return 0;

            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED && onSize) {
                    onSize(hwnd, LOWORD(lParam), HIWORD(lParam));
                }
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        LRESULT ProcessMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam, HWND hwnd,
            const std::function<void()>& onLeftDown = nullptr,
            const std::function<void()>& onLeftUp = nullptr) {
            switch (msg) {
            case WM_MOUSEMOVE:
                HandleMouseMoveBase(lParam);
                return 0;

            case WM_LBUTTONDOWN:
                HandleMouseButtonDownBase(msg, hwnd);
                if (onLeftDown) onLeftDown();
                return 0;

            case WM_LBUTTONUP:
                HandleMouseButtonUpBase(msg);
                if (onLeftUp) onLeftUp();
                return 0;

            case WM_RBUTTONDOWN:
                HandleMouseButtonDownBase(msg, hwnd);
                return 0;

            case WM_RBUTTONUP:
                HandleMouseButtonUpBase(msg);
                return 0;

            case WM_MBUTTONDOWN:
                HandleMouseButtonDownBase(msg, hwnd);
                return 0;

            case WM_MBUTTONUP:
                HandleMouseButtonUpBase(msg);
                return 0;

            case WM_MOUSEWHEEL:
                HandleMouseWheelBase(wParam);
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        MouseState m_mouseState{};
        int m_lastResizeWidth = 0;
        int m_lastResizeHeight = 0;
    };

}

#endif