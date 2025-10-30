#include "UIMessageHandler.h"

#include "App/ControllerCore.h"
#include "Common/EventBus.h"
#include "UI/Core/UIManager.h"
#include "WindowManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    UIMessageHandler::UIMessageHandler(
        ControllerCore* controller,
        WindowManager* windowManager,
        UIManager* uiManager,
        EventBus* bus
    )
        : MessageHandlerBase()
        , m_controller(controller)
        , m_windowManager(windowManager)
        , m_uiManager(uiManager)
        , m_bus(bus) {
        if (!controller || !windowManager || !uiManager || !bus) {
            throw std::invalid_argument("UIMessageHandler: All dependencies must be non-null.");
        }
    }

    LRESULT UIMessageHandler::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (m_uiManager->HandleMessage(hwnd, msg, wParam, lParam)) {
            return 0;
        }

        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SIZE:
            return ProcessResizeMessage(hwnd, msg, wParam, lParam,
                [this] { m_windowManager->OnUIResizeStart(); },
                [this](HWND h) { m_windowManager->OnUIResizeEnd(h); },
                [this](HWND h, int w, int h2) {
                    m_windowManager->OnUIResize(h, w, h2);
                }
            );

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
            return ProcessMouseMessage(msg, wParam, lParam, hwnd);

        case WM_CLOSE:
            HandleClose(hwnd);
            return 0;

        case WM_DESTROY:
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    void UIMessageHandler::HandleClose(HWND hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }

}