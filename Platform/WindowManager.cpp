// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WindowManager, responsible for creating and managing
// the application's windows and their associated resources.
//
// This implementation handles the complete lifecycle of both the main window
// and overlay window, managing transitions between modes, recreating graphics
// contexts on device loss, and routing Win32 messages to appropriate handlers.
// It maintains mouse input state and propagates resize events to all dependent
// subsystems.
//
// Key Implementation Details:
// - Two-window system: main (normal mode) and overlay (transparent mode)
// - Graphics context recreation on WM_SIZE and device loss
// - Mouse state tracking for state-driven input model
// - Event-driven overlay toggling through EventBus
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WindowManager.h"
#include "ControllerCore.h"
#include "EventBus.h"
#include "RenderEngine.h"
#include "Canvas.h"
#include "IRenderer.h"
#include "MainWindow.h"
#include "RendererManager.h"
#include "UIManager.h"
#include "WindowHelper.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WindowManager::WindowManager(
        HINSTANCE hInstance,
        ControllerCore* controller,
        EventBus* bus
    ) :
        m_hInstance(hInstance),
        m_controller(controller),
        m_isOverlay(false),
        m_mouseState{}
    {
        m_uiManager = std::make_unique<UIManager>(m_controller, this);
        SubscribeToEvents(bus);
    }

    WindowManager::~WindowManager() noexcept = default;

    [[nodiscard]] bool WindowManager::Initialize()
    {
        if (!InitializeMainWindow()) return false;
        if (!InitializeOverlayWindow()) return false;
        if (!RecreateGraphicsAndNotify(m_mainWnd->GetHwnd())) return false;

        WindowUtils::CenterOnScreen(m_mainWnd->GetHwnd());
        m_mainWnd->Show();

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ProcessMessages()
    {
        if (!m_mainWnd) return;
        if (!m_mainWnd->IsRunning()) return;

        m_mainWnd->ProcessMessages();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT WindowManager::HandleWindowMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        switch (msg)
        {
        case WM_CLOSE:
            OnExitRequest();
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
                PropagateResizeToSubsystems(hwnd);
            return 0;

        case WM_MOUSEMOVE:
        {
            int x = 0;
            int y = 0;
            WindowUtils::ExtractMousePos(lParam, x, y);
            m_mouseState.position.x = static_cast<float>(x);
            m_mouseState.position.y = static_cast<float>(y);
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

        case WM_MBUTTONDOWN:
            m_mouseState.middleButtonDown = true;
            return 0;

        case WM_MBUTTONUP:
            m_mouseState.middleButtonDown = false;
            return 0;

        case WM_MOUSEWHEEL:
            m_mouseState.wheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            return 0;

        case WM_NCHITTEST:
            if (m_isOverlay)
                return HTCAPTION;
            break;

        case WM_ERASEBKGND:
            return 1;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ToggleOverlay()
    {
        m_isOverlay = !m_isOverlay;

        if (m_isOverlay)
            ActivateOverlayMode();
        else
            DeactivateOverlayMode();

        NotifyRendererOfModeChange();
    }

    bool WindowManager::RecreateGraphicsAndNotify(HWND hwnd)
    {
        if (!hwnd) return false;
        if (!RecreateGraphicsContext(hwnd)) return false;

        PropagateResizeToSubsystems(hwnd);
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool WindowManager::IsRunning() const
    {
        return m_mainWnd && m_mainWnd->IsRunning();
    }

    [[nodiscard]] bool WindowManager::IsOverlayMode() const noexcept
    {
        return m_isOverlay;
    }

    [[nodiscard]] bool WindowManager::IsActive() const
    {
        if (!IsRunning()) return false;

        const HWND hwnd = GetCurrentHwnd();
        if (!IsWindow(hwnd)) return false;
        if (!IsWindowVisible(hwnd)) return false;
        if (IsIconic(hwnd)) return false;

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] RenderEngine* WindowManager::GetRenderEngine() const noexcept
    {
        return m_engine.get();
    }

    [[nodiscard]] UIManager* WindowManager::GetUIManager() const noexcept
    {
        return m_uiManager.get();
    }

    [[nodiscard]] MainWindow* WindowManager::GetMainWindow() const noexcept
    {
        return m_mainWnd.get();
    }

    [[nodiscard]] HWND WindowManager::GetCurrentHwnd() const
    {
        if (m_isOverlay)
            return m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr;

        return m_mainWnd ? m_mainWnd->GetHwnd() : nullptr;
    }

    [[nodiscard]] const WindowManager::MouseState& WindowManager::GetMouseState() const noexcept
    {
        return m_mouseState;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::SubscribeToEvents(EventBus* bus)
    {
        if (!bus) return;

        bus->Subscribe(InputAction::ToggleOverlay, [this]() {
            ToggleOverlay();
            });

        bus->Subscribe(InputAction::Exit, [this]() {
            OnExitRequest();
            });
    }

    [[nodiscard]] bool WindowManager::InitializeMainWindow()
    {
        m_mainWnd = std::make_unique<MainWindow>(m_hInstance);
        if (!m_mainWnd) return false;

        return m_mainWnd->Initialize(
            L"Spectrum Visualizer",
            800,
            600,
            false,
            this
        );
    }

    [[nodiscard]] bool WindowManager::InitializeOverlayWindow()
    {
        const auto screenSize = WindowUtils::GetScreenSize();

        m_overlayWnd = std::make_unique<MainWindow>(m_hInstance);
        if (!m_overlayWnd) return false;

        return m_overlayWnd->Initialize(
            L"Spectrum Overlay",
            screenSize.w,
            300,
            true,
            this
        );
    }

    void WindowManager::ActivateOverlayMode()
    {
        LOG_INFO("=== ACTIVATING OVERLAY MODE ===");
        HideMainWindow();
        PositionAndShowOverlay();

        if (m_overlayWnd)
        {
            HWND hwnd = m_overlayWnd->GetHwnd();
            LOG_INFO("Recreating graphics for overlay HWND: " << hwnd);
            RecreateGraphicsAndNotify(hwnd);
        }
        LOG_INFO("=== OVERLAY MODE ACTIVATED ===");
    }

    void WindowManager::DeactivateOverlayMode()
    {
        HideOverlayWindow();
        ShowMainWindow();

        if (m_mainWnd)
            RecreateGraphicsAndNotify(m_mainWnd->GetHwnd());
    }

    void WindowManager::HideMainWindow() const
    {
        if (!m_mainWnd) return;
        m_mainWnd->Hide();
    }

    void WindowManager::ShowMainWindow() const
    {
        if (!m_mainWnd) return;

        m_mainWnd->Show();
        SetForegroundWindow(m_mainWnd->GetHwnd());
    }

    void WindowManager::HideOverlayWindow() const
    {
        if (!m_overlayWnd) return;
        m_overlayWnd->Hide();
    }

    void WindowManager::PositionAndShowOverlay() const
    {
        if (!m_overlayWnd) return;

        const HWND hwnd = m_overlayWnd->GetHwnd();
        const auto screenSize = WindowUtils::GetScreenSize();
        const int overlayHeight = m_overlayWnd->GetHeight();
        const int overlayWidth = m_overlayWnd->GetWidth();

        SetWindowPos(
            hwnd,
            HWND_TOPMOST,
            0,
            screenSize.h - overlayHeight,
            overlayWidth,
            overlayHeight,
            SWP_SHOWWINDOW
        );

        InvalidateRect(hwnd, nullptr, FALSE);
    }

    bool WindowManager::RecreateGraphicsContext(HWND hwnd)
    {
        m_engine.reset();

        m_engine = std::make_unique<RenderEngine>(hwnd, m_isOverlay);

        if (!m_engine) return false;
        if (!m_engine->Initialize()) return false;

        return true;
    }

    void WindowManager::PropagateResizeToSubsystems(HWND hwnd)
    {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        const int width = clientRect.right - clientRect.left;
        const int height = clientRect.bottom - clientRect.top;

        if (m_engine)
            m_engine->Resize(width, height);

        if (m_uiManager && m_engine)
            m_uiManager->RecreateResources(m_engine->GetCanvas(), width, height);

        if (m_controller)
            m_controller->OnResize(width, height);
    }

    void WindowManager::NotifyRendererOfModeChange() const
    {
        if (!m_controller) return;

        auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager) return;

        auto* renderer = rendererManager->GetCurrentRenderer();
        if (!renderer) return;

        renderer->SetOverlayMode(m_isOverlay);
    }

    void WindowManager::OnExitRequest()
    {
        if (IsOverlayMode())
            ToggleOverlay();
        else if (m_controller)
            m_controller->OnCloseRequest();
    }

} // namespace Spectrum