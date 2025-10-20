// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WindowManager, responsible for creating and managing
// the application's windows and their associated resources.
//
// Implements the WindowManager by defining the initialization of main and
// overlay windows, and the logic for switching between them. It serves as
// the primary router for Win32 messages, delegating them to the appropriate
// high-level systems.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WindowManager.h"
#include "ControllerCore.h"
#include "EventBus.h"
#include "GraphicsContext.h"
#include "IRenderer.h"
#include "MainWindow.h"
#include "RendererManager.h"
#include "UIManager.h"
#include "WindowHelper.h"

namespace Spectrum {

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

    bool WindowManager::RecreateGraphicsAndNotify(HWND hwnd)
    {
        if (!hwnd) return false;
        if (!RecreateGraphicsContext(hwnd)) return false;

        PropagateResizeToSubsystems(hwnd);
        return true;
    }

    void WindowManager::ProcessMessages()
    {
        if (m_mainWnd && m_mainWnd->IsRunning())
            m_mainWnd->ProcessMessages();
    }

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
            {
                PropagateResizeToSubsystems(hwnd);
            }
            return 0;

        case WM_MOUSEMOVE:
        {
            int x, y;
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

        case WM_NCHITTEST:
            if (m_isOverlay)
                return HTCAPTION;
            break;

        case WM_ERASEBKGND:
            return 1;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void WindowManager::ToggleOverlay()
    {
        m_isOverlay = !m_isOverlay;
        if (m_isOverlay)
            ActivateOverlayMode();
        else
            DeactivateOverlayMode();

        NotifyRendererOfModeChange();
    }

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
        return IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd);
    }

    [[nodiscard]] GraphicsContext* WindowManager::GetGraphics() const noexcept
    {
        return m_graphics.get();
    }

    [[nodiscard]] UIManager* WindowManager::GetUIManager() const noexcept
    {
        return m_uiManager.get();
    }

    [[nodiscard]] HWND WindowManager::GetCurrentHwnd() const
    {
        return m_isOverlay
            ? (m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr)
            : (m_mainWnd ? m_mainWnd->GetHwnd() : nullptr);
    }

    [[nodiscard]] MainWindow* WindowManager::GetMainWindow() const noexcept
    {
        return m_mainWnd.get();
    }

    void WindowManager::SubscribeToEvents(EventBus* bus)
    {
        bus->Subscribe(InputAction::ToggleOverlay, [this]() { this->ToggleOverlay(); });
        bus->Subscribe(InputAction::Exit, [this]() { this->OnExitRequest(); });
    }

    [[nodiscard]] bool WindowManager::InitializeMainWindow()
    {
        m_mainWnd = std::make_unique<MainWindow>(m_hInstance);
        return m_mainWnd->Initialize(L"Spectrum Visualizer", 800, 600, false, this);
    }

    [[nodiscard]] bool WindowManager::InitializeOverlayWindow()
    {
        const auto screenSize = WindowUtils::GetScreenSize();
        m_overlayWnd = std::make_unique<MainWindow>(m_hInstance);
        return m_overlayWnd->Initialize(L"Spectrum Overlay", screenSize.w, 300, true, this);
    }

    void WindowManager::ActivateOverlayMode()
    {
        HideMainWindow();
        PositionAndShowOverlay();
        (void)RecreateGraphicsAndNotify(m_overlayWnd->GetHwnd());
    }

    void WindowManager::DeactivateOverlayMode()
    {
        HideOverlayWindow();
        ShowMainWindow();
        (void)RecreateGraphicsAndNotify(m_mainWnd->GetHwnd());
    }

    void WindowManager::HideMainWindow() const
    {
        if (m_mainWnd) m_mainWnd->Hide();
    }

    void WindowManager::PositionAndShowOverlay() const
    {
        if (!m_overlayWnd) return;

        HWND newHwnd = m_overlayWnd->GetHwnd();
        const auto screenSize = WindowUtils::GetScreenSize();
        const int overlayH = m_overlayWnd->GetHeight();

        SetWindowPos(
            newHwnd,
            HWND_TOPMOST,
            0,
            screenSize.h - overlayH,
            0, 0,
            SWP_NOSIZE | SWP_SHOWWINDOW
        );
    }

    void WindowManager::HideOverlayWindow() const
    {
        if (m_overlayWnd) m_overlayWnd->Hide();
    }

    void WindowManager::ShowMainWindow() const
    {
        if (!m_mainWnd) return;
        m_mainWnd->Show();
        SetForegroundWindow(m_mainWnd->GetHwnd());
    }

    [[nodiscard]] bool WindowManager::RecreateGraphicsContext(HWND hwnd)
    {
        m_graphics.reset();
        m_graphics = std::make_unique<GraphicsContext>(hwnd);
        if (!m_graphics->Initialize()) return false;
        return true;
    }

    void WindowManager::PropagateResizeToSubsystems(HWND hwnd)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        if (m_graphics)
        {
            m_graphics->Resize(width, height);
        }

        if (m_uiManager && m_graphics)
        {
            m_uiManager->RecreateResources(*m_graphics, width, height);
        }

        if (m_controller)
        {
            m_controller->OnResize(width, height);
        }
    }

    void WindowManager::NotifyRendererOfModeChange() const
    {
        if (m_controller)
            if (auto* rendererManager = m_controller->GetRendererManager())
                if (auto* renderer = rendererManager->GetCurrentRenderer())
                    renderer->SetOverlayMode(m_isOverlay);
    }

    void WindowManager::OnExitRequest()
    {
        if (IsOverlayMode())
            ToggleOverlay();
        else if (m_controller)
            m_controller->OnCloseRequest();
    }

}