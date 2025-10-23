// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WindowManager, responsible for creating and managing
// the application's windows and their associated resources.
//
// This implementation handles the complete lifecycle of both the main window
// and overlay window, managing transitions between modes and recreating
// graphics contexts. It delegates all message handling to MessageHandler.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WindowManager.h"
#include "App/ControllerCore.h"
#include "Graphics/Api/Core/RenderEngine.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "MainWindow.h"
#include "MessageHandler.h"
#include "Win32Utils.h"
#include "UI/Core/UIManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WindowManager::WindowManager(HINSTANCE hInstance, ControllerCore* controller, EventBus* bus) :
        m_hInstance(hInstance),
        m_controller(controller),
        m_isOverlay(false)
    {
        if (!m_controller) throw std::invalid_argument("controller dependency cannot be null");

        m_uiManager = std::make_unique<UIManager>(m_controller, this);
        m_messageHandler = std::make_unique<MessageHandler>(controller, this, m_uiManager.get(), bus);
    }

    WindowManager::~WindowManager() noexcept = default;

    [[nodiscard]] bool WindowManager::Initialize()
    {
        if (!InitializeMainWindow()) return false;
        if (!InitializeOverlayWindow()) return false;
        if (!RecreateGraphicsAndNotify(m_mainWnd->GetHwnd())) return false;

        const auto screenSize = Win32Utils::GetScreenSize();
        RECT windowRect;
        GetWindowRect(m_mainWnd->GetHwnd(), &windowRect);
        const auto windowSize = Win32Utils::Size{ windowRect.right - windowRect.left, windowRect.bottom - windowRect.top };
        const auto centerPos = Win32Utils::CalculateCenterPosition(windowSize, screenSize);

        SetWindowPos(
            m_mainWnd->GetHwnd(), nullptr, centerPos.x, centerPos.y,
            0, 0, SWP_NOSIZE | SWP_NOZORDER
        );
        m_mainWnd->Show();

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ProcessMessages()
    {
        if (m_mainWnd && m_mainWnd->IsRunning()) m_mainWnd->ProcessMessages();
    }

    void WindowManager::PropagateResizeToSubsystems(HWND hwnd)
    {
        if (!hwnd) return;

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        const int width = clientRect.right - clientRect.left;
        const int height = clientRect.bottom - clientRect.top;

        if (width == 0 || height == 0) return;

        if (m_engine) m_engine->Resize(width, height);
        if (m_uiManager && m_engine) m_uiManager->RecreateResources(m_engine->GetCanvas(), width, height);
        if (m_controller) m_controller->OnResize(width, height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ToggleOverlay()
    {
        m_isOverlay = !m_isOverlay;

        if (m_isOverlay) ActivateOverlayMode();
        else DeactivateOverlayMode();

        NotifyRendererOfModeChange();
    }

    [[nodiscard]] bool WindowManager::RecreateGraphicsAndNotify(HWND hwnd)
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

    [[nodiscard]] MessageHandler* WindowManager::GetMessageHandler() const noexcept
    {
        return m_messageHandler.get();
    }

    [[nodiscard]] MainWindow* WindowManager::GetMainWindow() const noexcept
    {
        return m_mainWnd.get();
    }

    [[nodiscard]] HWND WindowManager::GetCurrentHwnd() const
    {
        if (m_isOverlay) return m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr;
        return m_mainWnd ? m_mainWnd->GetHwnd() : nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool WindowManager::InitializeMainWindow()
    {
        m_mainWnd = std::make_unique<MainWindow>(m_hInstance);
        if (!m_mainWnd) return false;

        return m_mainWnd->Initialize(L"Spectrum Visualizer", 800, 600, false, m_messageHandler.get());
    }

    [[nodiscard]] bool WindowManager::InitializeOverlayWindow()
    {
        const auto screenSize = Win32Utils::GetScreenSize();
        m_overlayWnd = std::make_unique<MainWindow>(m_hInstance);
        if (!m_overlayWnd) return false;

        return m_overlayWnd->Initialize(L"Spectrum Overlay", screenSize.w, 300, true, m_messageHandler.get());
    }

    void WindowManager::ActivateOverlayMode()
    {
        HideMainWindow();
        PositionAndShowOverlay();

        if (m_overlayWnd)
        {
            if (!RecreateGraphicsAndNotify(m_overlayWnd->GetHwnd()))
            {
                LOG_ERROR("Failed to recreate graphics context for overlay window");
            }
        }
    }

    void WindowManager::DeactivateOverlayMode()
    {
        HideOverlayWindow();
        ShowMainWindow();

        if (m_mainWnd)
        {
            if (!RecreateGraphicsAndNotify(m_mainWnd->GetHwnd()))
            {
                LOG_ERROR("Failed to recreate graphics context for main window");
            }
        }
    }

    void WindowManager::HideMainWindow() const
    {
        if (m_mainWnd) m_mainWnd->Hide();
    }

    void WindowManager::ShowMainWindow() const
    {
        if (!m_mainWnd) return;
        m_mainWnd->Show();
        SetForegroundWindow(m_mainWnd->GetHwnd());
    }

    void WindowManager::HideOverlayWindow() const
    {
        if (m_overlayWnd) m_overlayWnd->Hide();
    }

    void WindowManager::PositionAndShowOverlay() const
    {
        if (!m_overlayWnd) return;

        const HWND hwnd = m_overlayWnd->GetHwnd();
        const auto screenSize = Win32Utils::GetScreenSize();
        const int overlayHeight = m_overlayWnd->GetHeight();

        SetWindowPos(
            hwnd, HWND_TOPMOST, 0, screenSize.h - overlayHeight,
            screenSize.w, overlayHeight, SWP_SHOWWINDOW
        );
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    [[nodiscard]] bool WindowManager::RecreateGraphicsContext(HWND hwnd)
    {
        m_engine = std::make_unique<RenderEngine>(hwnd, m_isOverlay);
        return m_engine && m_engine->Initialize();
    }

    void WindowManager::NotifyRendererOfModeChange() const
    {
        if (!m_controller) return;
        if (auto* rendererManager = m_controller->GetRendererManager())
        {
            if (auto* renderer = rendererManager->GetCurrentRenderer())
            {
                renderer->SetOverlayMode(m_isOverlay);
            }
        }
    }

} // namespace Spectrum::Platform