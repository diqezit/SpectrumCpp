// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the MessageHandler class, which processes Win32 window
// messages.
//
// This implementation contains the main message switch, updating mouse state
// and propagating system events like window resizing or close requests to
// the appropriate managers. It also subscribes to application-level events
// to react to user actions.
//
// Resize events are optimized with WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE
// to minimize resource recreation during active window resizing.
//
// Implementation notes:
// - Message handling split into categories for better maintainability
// - Resize debouncing prevents redundant operations
// - Each message type handled by dedicated method (SRP)
// - No business logic in header file
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
        ValidateDependencies();
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
        // Give UI priority to handle input messages
        if (m_uiManager->HandleMessage(hwnd, msg, wParam, lParam)) {
            return 0;
        }

        // Route to appropriate handler based on message category
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
    // Dependency Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::ValidateDependencies() const
    {
        ValidateDependency(m_controller, "controller");
        ValidateDependency(m_windowManager, "windowManager");
        ValidateDependency(m_uiManager, "uiManager");
    }

    void MessageHandler::ValidateDependency(void* ptr, const char* name) const
    {
        if (!ptr) {
            std::string errorMsg = std::string(name) + " dependency cannot be null";
            throw std::invalid_argument(errorMsg);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Message Category Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT MessageHandler::HandleLifecycleMessage(UINT msg)
    {
        switch (msg)
        {
        case WM_CLOSE:
            HandleCloseRequest();
            return 0;

        case WM_DESTROY:
            HandleDestroyRequest();
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
            HandleResizeStart();
            return 0;

        case WM_EXITSIZEMOVE:
            HandleResizeEnd(hwnd);
            return 0;

        case WM_SIZE:
            HandleResize(hwnd, wParam, lParam);
            return 0;

        default:
            return 0;
        }
    }

    LRESULT MessageHandler::HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_MOUSEMOVE:
            HandleMouseMove(lParam);
            return 0;

        case WM_LBUTTONDOWN:
            HandleLeftButtonDown();
            return 0;

        case WM_LBUTTONUP:
            HandleLeftButtonUp();
            return 0;

        case WM_RBUTTONDOWN:
            HandleRightButtonDown();
            return 0;

        case WM_RBUTTONUP:
            HandleRightButtonUp();
            return 0;

        case WM_MOUSEWHEEL:
            HandleMouseWheel(wParam);
            return 0;

        default:
            return 0;
        }
    }

    LRESULT MessageHandler::HandleSpecialMessage(UINT msg)
    {
        switch (msg)
        {
        case WM_NCHITTEST:
            return HandleHitTest();

        case WM_ERASEBKGND:
            return HandleEraseBackground();

        default:
            return 0;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Message Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::HandleCloseRequest()
    {
        OnExitRequest();
    }

    void MessageHandler::HandleDestroyRequest()
    {
        PostQuitMessage(0);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resize Message Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::HandleResizeStart()
    {
        m_windowManager->OnResizeStart();
    }

    void MessageHandler::HandleResizeEnd(HWND hwnd)
    {
        m_windowManager->OnResizeEnd(hwnd);
    }

    void MessageHandler::HandleResize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (IsMinimized(wParam)) {
            return;
        }

        int width = 0;
        int height = 0;
        ExtractResizeDimensions(lParam, width, height);

        if (!ShouldProcessResize(width, height)) {
            return;
        }

        UpdateResizeCache(width, height);
        m_windowManager->OnResize(hwnd, width, height);
    }

    bool MessageHandler::IsMinimized(WPARAM wParam) const
    {
        return wParam == SIZE_MINIMIZED;
    }

    bool MessageHandler::ShouldProcessResize(int width, int height)
    {
        return width != m_lastResizeWidth || height != m_lastResizeHeight;
    }

    void MessageHandler::ExtractResizeDimensions(
        LPARAM lParam,
        int& outWidth,
        int& outHeight
    ) const
    {
        outWidth = LOWORD(lParam);
        outHeight = HIWORD(lParam);
    }

    void MessageHandler::UpdateResizeCache(int width, int height)
    {
        m_lastResizeWidth = width;
        m_lastResizeHeight = height;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mouse Message Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::HandleMouseMove(LPARAM lParam)
    {
        UpdateMousePosition(lParam);
    }

    void MessageHandler::HandleLeftButtonDown()
    {
        SetLeftButtonState(true);
    }

    void MessageHandler::HandleLeftButtonUp()
    {
        SetLeftButtonState(false);
    }

    void MessageHandler::HandleRightButtonDown()
    {
        SetRightButtonState(true);
    }

    void MessageHandler::HandleRightButtonUp()
    {
        SetRightButtonState(false);
    }

    void MessageHandler::HandleMouseWheel(WPARAM wParam)
    {
        UpdateWheelDelta(wParam);
    }

    void MessageHandler::UpdateMousePosition(LPARAM lParam)
    {
        int x = 0;
        int y = 0;
        Win32Utils::ExtractMousePos(lParam, x, y);

        m_mouseState.position = {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }

    void MessageHandler::SetLeftButtonState(bool pressed)
    {
        m_mouseState.leftButtonDown = pressed;
    }

    void MessageHandler::SetRightButtonState(bool pressed)
    {
        m_mouseState.rightButtonDown = pressed;
    }

    void MessageHandler::UpdateWheelDelta(WPARAM wParam)
    {
        const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        m_mouseState.wheelDelta = static_cast<float>(delta) / WHEEL_DELTA;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Special Message Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT MessageHandler::HandleHitTest()
    {
        if (m_windowManager->IsOverlayMode()) {
            return HTCAPTION;
        }

        return HTCLIENT;
    }

    LRESULT MessageHandler::HandleEraseBackground()
    {
        // Prevent flickering by indicating we handle background erase
        return 1;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event Subscription
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::SubscribeToEvents(EventBus* bus)
    {
        if (!bus) {
            return;
        }

        SubscribeToOverlayToggle(bus);
        SubscribeToExitAction(bus);
    }

    void MessageHandler::SubscribeToOverlayToggle(EventBus* bus)
    {
        bus->Subscribe(InputAction::ToggleOverlay, [this]() {
            m_windowManager->ToggleOverlay();
            });
    }

    void MessageHandler::SubscribeToExitAction(EventBus* bus)
    {
        bus->Subscribe(InputAction::Exit, [this]() {
            OnExitRequest();
            });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Exit Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MessageHandler::OnExitRequest()
    {
        if (m_windowManager->IsOverlayMode()) {
            HandleOverlayExit();
        }
        else {
            HandleNormalExit();
        }
    }

    void MessageHandler::HandleOverlayExit()
    {
        m_windowManager->ToggleOverlay();
    }

    void MessageHandler::HandleNormalExit()
    {
        m_controller->OnCloseRequest();
    }

} // namespace Spectrum::Platform