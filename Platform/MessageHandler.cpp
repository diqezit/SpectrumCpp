#include "MessageHandler.h"
#include "App/ControllerCore.h"
#include "Common/EventBus.h"
#include "WindowManager.h"
#include "Win32Utils.h"
#include "UI/Core/UIManager.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <stdexcept>

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    MessageHandler::MessageHandler(
        ControllerCore* controller,
        WindowManager* windowManager,
        UIManager* uiManager,
        EventBus* bus
    )
        : m_controller(controller)
        , m_windowManager(windowManager)
        , m_uiManager(uiManager)
    {
        if (!Spectrum::Helpers::Validate::Pointer(m_controller, "controller", "MessageHandler")) {
            throw std::invalid_argument("controller dependency cannot be null");
        }
        if (!Spectrum::Helpers::Validate::Pointer(m_windowManager, "windowManager", "MessageHandler")) {
            throw std::invalid_argument("windowManager dependency cannot be null");
        }

        SubscribeToEvents(bus);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT MessageHandler::HandleWindowMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        if (m_uiManager && m_uiManager->HandleMessage(hwnd, msg, wParam, lParam)) {
            return 0;
        }

        switch (msg)
        {
        case WM_CLOSE:
        case WM_DESTROY:
            return HandleLifecycleMessage(msg);

        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SIZE:
            return HandleResizeMessage(hwnd, msg, wParam, lParam);

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL:
            return HandleMouseMessage(msg, wParam, lParam);

        case WM_NCHITTEST:
        case WM_ERASEBKGND:
            return HandleSpecialMessage(msg);

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    const MessageHandler::MouseState& MessageHandler::GetMouseState() const noexcept
    {
        return m_mouseState;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Message Category Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT MessageHandler::HandleLifecycleMessage(UINT msg)
    {
        switch (msg)
        {
        case WM_CLOSE:
            if (m_windowManager->IsOverlayMode()) {
                m_windowManager->ToggleOverlay();
            }
            else {
                m_controller->OnCloseRequest();
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return 0;
        }
    }

    LRESULT MessageHandler::HandleResizeMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        switch (msg)
        {
        case WM_ENTERSIZEMOVE:
            m_windowManager->OnResizeStart();
            return 0;

        case WM_EXITSIZEMOVE:
            m_windowManager->OnResizeEnd(hwnd);
            return 0;

        case WM_SIZE:
        {
            if (wParam == SIZE_MINIMIZED) {
                return 0;
            }

            const int width = LOWORD(lParam);
            const int height = HIWORD(lParam);

            if (width != m_lastResizeWidth || height != m_lastResizeHeight) {
                m_lastResizeWidth = width;
                m_lastResizeHeight = height;
                m_windowManager->OnResize(hwnd, width, height);
            }
            return 0;
        }

        default:
            return 0;
        }
    }

    LRESULT MessageHandler::HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_MOUSEMOVE:
        {
            int x = 0;
            int y = 0;
            Win32Utils::ExtractMousePos(lParam, x, y);
            m_mouseState.position = { static_cast<float>(x), static_cast<float>(y) };
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            m_mouseState.leftButtonDown = true;
            if (m_controller)
            {
                m_controller->OnMainWindowClick(m_mouseState.position);
            }
            return 0;
        }

        case WM_LBUTTONUP:
            m_mouseState.leftButtonDown = false;
            return 0;

        case WM_RBUTTONDOWN:
            m_mouseState.rightButtonDown = true;
            return 0;

        case WM_RBUTTONUP:
            m_mouseState.rightButtonDown = false;
            return 0;

        case WM_MOUSEWHEEL:
        {
            const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            m_mouseState.wheelDelta = static_cast<float>(delta) / WHEEL_DELTA;
            return 0;
        }

        default:
            return 0;
        }
    }

    LRESULT MessageHandler::HandleSpecialMessage(UINT msg)
    {
        switch (msg)
        {
        case WM_NCHITTEST:
            return m_windowManager->IsOverlayMode() ? HTCAPTION : HTCLIENT;

        case WM_ERASEBKGND:
            return 1;

        default:
            return 0;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event Subscription
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::SubscribeToEvents(EventBus* bus)
    {
        VALIDATE_PTR_OR_RETURN(bus, "MessageHandler");

        bus->Subscribe(InputAction::ToggleOverlay, [this]() {
            m_windowManager->ToggleOverlay();
            });

        bus->Subscribe(InputAction::Exit, [this]() {
            if (m_windowManager->IsOverlayMode()) {
                m_windowManager->ToggleOverlay();
            }
            else {
                m_controller->OnCloseRequest();
            }
            });
    }

} // namespace Spectrum::Platform