#include "MessageHandler.h"

#include "App/ControllerCore.h"
#include "Common/EventBus.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Win32Utils.h"
#include "WindowManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    MessageHandler::MessageHandler(
        ControllerCore* controller,
        WindowManager* windowManager,
        EventBus* bus
    )
        : MessageHandlerBase()
        , m_controller(controller)
        , m_windowManager(windowManager) {
        if (!Helpers::Validate::Pointer(m_controller, "controller", "MessageHandler")) {
            throw std::invalid_argument("controller dependency cannot be null");
        }
        if (!Helpers::Validate::Pointer(m_windowManager, "windowManager", "MessageHandler")) {
            throw std::invalid_argument("windowManager dependency cannot be null");
        }

        SubscribeToEvents(bus);
    }

    LRESULT MessageHandler::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            return HandleLifecycleMessage(msg);

        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SIZE:
            return ProcessResizeMessage(hwnd, msg, wParam, lParam,
                [this] { m_windowManager->OnResizeStart(); },
                [this](HWND h) { m_windowManager->OnResizeEnd(h); },
                [this](HWND h, int w, int h2) {
                    if (w != m_lastResizeWidth || h2 != m_lastResizeHeight) {
                        m_lastResizeWidth = w;
                        m_lastResizeHeight = h2;
                        m_windowManager->OnResize(h, w, h2);
                    }
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
            return ProcessMouseMessage(msg, wParam, lParam, hwnd,
                [this] {
                    if (m_controller) {
                        m_controller->OnMainWindowClick(m_mouseState.position);
                    }
                }
            );

        case WM_NCHITTEST:
        case WM_ERASEBKGND:
            return HandleSpecialMessage(msg);

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    LRESULT MessageHandler::HandleLifecycleMessage(UINT msg) {
        switch (msg) {
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

    LRESULT MessageHandler::HandleSpecialMessage(UINT msg) {
        switch (msg) {
        case WM_NCHITTEST:
            return m_windowManager->IsOverlayMode() ? HTCAPTION : HTCLIENT;

        case WM_ERASEBKGND:
            return 1;

        default:
            return 0;
        }
    }

    void MessageHandler::SubscribeToEvents(EventBus* bus) {
        VALIDATE_PTR_OR_RETURN(bus, "MessageHandler");

        try {
            bus->Subscribe(InputAction::ToggleOverlay, [this] {
                m_windowManager->ToggleOverlay();
                });

            bus->Subscribe(InputAction::Exit, [this] {
                if (m_windowManager->IsOverlayMode()) {
                    m_windowManager->ToggleOverlay();
                }
                else {
                    m_controller->OnCloseRequest();
                }
                });

            LOG_INFO("MessageHandler: Event subscription completed");
        }
        catch (const std::exception&) {
            LOG_ERROR("MessageHandler: Event subscription failed");
        }
    }

}