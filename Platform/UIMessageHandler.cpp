// UIMessageHandler.cpp
#include "UIMessageHandler.h"
#include "App/ControllerCore.h"
#include "UI/Core/UIManager.h"
#include "WindowManager.h"
#include "Common/EventBus.h"

namespace Spectrum::Platform {

    UIMessageHandler::UIMessageHandler(
        ControllerCore* controller,
        WindowManager* windowManager,
        UIManager* uiManager,
        EventBus* bus
    ) :
        m_controller(controller),
        m_windowManager(windowManager),
        m_uiManager(uiManager),
        m_bus(bus),
        m_mouseState{ }
    {
        if (!controller || !windowManager || !uiManager || !bus)
        {
            throw std::invalid_argument("UIMessageHandler: All dependencies must be non-null.");
        }
    }

    LRESULT UIMessageHandler::HandleWindowMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        if (m_uiManager->HandleMessage(hwnd, msg, wParam, lParam))
        {
            return 0;
        }

        switch (msg)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
            HandleResize(hwnd, wParam, lParam);
            return 0;

        case WM_ENTERSIZEMOVE:
            m_windowManager->OnUIResizeStart();
            return 0;

        case WM_EXITSIZEMOVE:
            m_windowManager->OnUIResizeEnd(hwnd);
            return 0;

        case WM_MOUSEMOVE:
            HandleMouseMove(lParam);
            return 0;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            HandleMouseDown(msg, hwnd);
            return 0;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            HandleMouseUp(msg);
            return 0;

        case WM_MOUSEWHEEL:
            HandleMouseWheel(wParam);
            return 0;

        case WM_CLOSE:
            HandleClose(hwnd);
            return 0;

        case WM_DESTROY:
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    const UIMouseState& UIMessageHandler::GetMouseState() const noexcept
    {
        return m_mouseState;
    }

    void UIMessageHandler::HandleResize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (wParam != SIZE_MINIMIZED)
        {
            m_windowManager->OnUIResize(hwnd, LOWORD(lParam), HIWORD(lParam));
        }
    }

    void UIMessageHandler::HandleMouseMove(LPARAM lParam)
    {
        m_mouseState.position.x = static_cast<float>(GET_X_LPARAM(lParam));
        m_mouseState.position.y = static_cast<float>(GET_Y_LPARAM(lParam));
    }

    void UIMessageHandler::HandleMouseDown(UINT msg, HWND hwnd)
    {
        SetCapture(hwnd);
        switch (msg)
        {
        case WM_LBUTTONDOWN: m_mouseState.leftButtonDown = true; break;
        case WM_RBUTTONDOWN: m_mouseState.rightButtonDown = true; break;
        case WM_MBUTTONDOWN: m_mouseState.middleButtonDown = true; break;
        }
    }

    void UIMessageHandler::HandleMouseUp(UINT msg)
    {
        ReleaseCapture();
        switch (msg)
        {
        case WM_LBUTTONUP:   m_mouseState.leftButtonDown = false; break;
        case WM_RBUTTONUP:   m_mouseState.rightButtonDown = false; break;
        case WM_MBUTTONUP:   m_mouseState.middleButtonDown = false; break;
        }
    }

    void UIMessageHandler::HandleMouseWheel(WPARAM wParam)
    {
        m_mouseState.wheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
    }

    void UIMessageHandler::HandleClose(HWND hwnd)
    {
        ShowWindow(hwnd, SW_HIDE);
    }

} // namespace Spectrum::Platform