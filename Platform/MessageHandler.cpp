// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the MessageHandler class, which processes Win32 window
// messages.
//
// This implementation contains the main message switch, updating mouse state
// and propagating system events like window resizing or close requests to
// the appropriate managers. It also subscribes to application-level events
// to react to user actions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "MessageHandler.h"
#include "App/ControllerCore.h"
#include "Common/EventBus.h"
#include "WindowManager.h"
#include "Win32Utils.h"
#include "UI/Core/UIManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    MessageHandler::MessageHandler(ControllerCore* controller, WindowManager* windowManager, UIManager* uiManager, EventBus* bus) :
        m_controller(controller),
        m_windowManager(windowManager),
        m_uiManager(uiManager)
    {
        if (!m_controller) throw std::invalid_argument("controller dependency cannot be null");
        if (!m_windowManager) throw std::invalid_argument("windowManager dependency cannot be null");
        if (!m_uiManager) throw std::invalid_argument("uiManager dependency cannot be null");

        SubscribeToEvents(bus);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT MessageHandler::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Give UI priority to handle input messages
        if (m_uiManager->HandleMessage(hwnd, msg, wParam, lParam))
        {
            return 0;
        }

        switch (msg)
        {
        case WM_CLOSE:
            OnExitRequest();
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) m_windowManager->PropagateResizeToSubsystems(hwnd);
            return 0;

        case WM_MOUSEMOVE:
        {
            int x = 0;
            int y = 0;
            Win32Utils::ExtractMousePos(lParam, x, y);
            m_mouseState.position = { static_cast<float>(x), static_cast<float>(y) };
            return 0;
        }

        case WM_LBUTTONDOWN:
            m_mouseState.leftButtonDown = true;
            return 0;

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
            m_mouseState.wheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            return 0;

        case WM_NCHITTEST:
            if (m_windowManager->IsOverlayMode()) return HTCAPTION;
            break;

        case WM_ERASEBKGND:
            return 1; // Prevent flickering by indicating we handle background erase
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] const MessageHandler::MouseState& MessageHandler::GetMouseState() const noexcept
    {
        return m_mouseState;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::SubscribeToEvents(EventBus* bus)
    {
        if (!bus) return;

        bus->Subscribe(InputAction::ToggleOverlay, [this]() {
            m_windowManager->ToggleOverlay();
            });

        bus->Subscribe(InputAction::Exit, [this]() {
            OnExitRequest();
            });
    }

    void MessageHandler::OnExitRequest()
    {
        if (m_windowManager->IsOverlayMode())
        {
            m_windowManager->ToggleOverlay();
        }
        else
        {
            m_controller->OnCloseRequest();
        }
    }

} // namespace Spectrum::Platform