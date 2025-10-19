// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Implements the WindowManager, responsible for creating and managing
// the application's windows and their associated resources.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "WindowManager.h"
#include "IRenderer.h"
#include "ControllerCore.h"
#include "WindowHelper.h"
#include "EventBus.h"
#include "GraphicsContext.h"
#include "UIManager.h"
#include "RendererManager.h"
#include "ColorPicker.h"

namespace Spectrum {

    WindowManager::WindowManager(
        HINSTANCE hInstance,
        ControllerCore* controller,
        EventBus* bus
    ) : m_hInstance(hInstance),
        m_controller(controller),
        m_isOverlay(false)
    {
        m_uiManager = std::make_unique<UIManager>(m_controller);
        SubscribeToEvents(bus);
    }

    WindowManager::~WindowManager() = default;

    void WindowManager::SubscribeToEvents(EventBus* bus) {
        bus->Subscribe(InputAction::ToggleOverlay, [this]() { this->ToggleOverlay(); });
        bus->Subscribe(InputAction::Exit, [this]() { this->OnExitRequest(); });
    }

    void WindowManager::OnExitRequest() {
        if (IsOverlayMode()) {
            ToggleOverlay();
        }
        else if (m_controller) {
            m_controller->OnClose();
        }
    }

    bool WindowManager::Initialize() {
        if (!InitializeMainWindow()) return false;
        if (!InitializeOverlayWindow()) return false;
        if (!RecreateGraphicsAndNotify(m_mainWnd->GetHwnd())) return false;
        if (!InitializeUI()) return false;

        WindowUtils::CenterOnScreen(m_mainWnd->GetHwnd());
        m_mainWnd->Show();
        return true;
    }

    bool WindowManager::InitializeMainWindow() {
        m_mainWnd = std::make_unique<MainWindow>(m_hInstance);
        return m_mainWnd->Initialize(
            L"Spectrum Visualizer", 800, 600, false, this
        );
    }

    bool WindowManager::InitializeOverlayWindow() {
        auto screenSize = WindowUtils::GetScreenSize();
        m_overlayWnd = std::make_unique<MainWindow>(m_hInstance);
        return m_overlayWnd->Initialize(
            L"Spectrum Overlay", screenSize.w, 300, true, this
        );
    }

    bool WindowManager::InitializeUI() {
        if (m_uiManager && !m_uiManager->Initialize(*m_graphics)) return false;
        return true;
    }

    void WindowManager::ProcessMessages() {
        if (m_mainWnd && m_mainWnd->IsRunning()) {
            m_mainWnd->ProcessMessages();
        }
    }

    bool WindowManager::IsRunning() const {
        return m_mainWnd && m_mainWnd->IsRunning();
    }

    bool WindowManager::IsActive() const {
        if (!IsRunning()) return false;
        HWND hwnd = GetCurrentHwnd();
        return IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd);
    }

    HWND WindowManager::GetCurrentHwnd() const {
        return m_isOverlay
            ? (m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr)
            : (m_mainWnd ? m_mainWnd->GetHwnd() : nullptr);
    }

    bool WindowManager::RecreateGraphicsContext(HWND hwnd) {
        m_graphics.reset();
        m_graphics = std::make_unique<GraphicsContext>(hwnd);
        if (!m_graphics->Initialize()) return false;
        return true;
    }

    void WindowManager::NotifyUIManagerOfResize() {
        if (m_uiManager) m_uiManager->RecreateResources(*m_graphics);
    }

    void WindowManager::NotifyControllerOfResize(HWND hwnd) {
        if (m_controller) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            m_controller->OnResize(rc.right - rc.left, rc.bottom - rc.top);
        }
    }

    void WindowManager::NotifySubsystemsOfResize(HWND hwnd) {
        NotifyUIManagerOfResize();
        NotifyControllerOfResize(hwnd);
    }

    bool WindowManager::RecreateGraphicsAndNotify(HWND hwnd) {
        if (!hwnd) return false;
        if (!RecreateGraphicsContext(hwnd)) return false;
        NotifySubsystemsOfResize(hwnd);
        return true;
    }

    IRenderer* WindowManager::GetActiveRenderer() {
        if (!m_controller) return nullptr;
        auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager) return nullptr;
        return rendererManager->GetCurrentRenderer();
    }

    void WindowManager::NotifyRendererOfModeChange() {
        if (auto* renderer = GetActiveRenderer()) {
            renderer->SetOverlayMode(m_isOverlay);
        }
    }

    void WindowManager::ToggleOverlay() {
        m_isOverlay = !m_isOverlay;
        if (m_isOverlay) {
            ActivateOverlayMode();
        }
        else {
            DeactivateOverlayMode();
        }
        NotifyRendererOfModeChange();
    }

    void WindowManager::HideMainUIAndWindow() {
        m_mainWnd->Hide();
        if (m_uiManager && m_uiManager->GetColorPicker()) {
            m_uiManager->GetColorPicker()->SetVisible(false);
        }
    }

    void WindowManager::PositionAndShowOverlayWindow() {
        HWND newHwnd = m_overlayWnd->GetHwnd();
        auto screenSize = WindowUtils::GetScreenSize();
        int overlayH = m_overlayWnd->GetHeight();
        SetWindowPos(newHwnd, HWND_TOPMOST, 0, screenSize.h - overlayH, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    void WindowManager::ActivateOverlayMode() {
        HideMainUIAndWindow();
        PositionAndShowOverlayWindow();
        RecreateGraphicsAndNotify(m_overlayWnd->GetHwnd());
    }

    void WindowManager::HideOverlayWindow() {
        m_overlayWnd->Hide();
    }

    void WindowManager::ShowMainUIAndWindow() {
        m_mainWnd->Show();
        if (m_uiManager && m_uiManager->GetColorPicker()) {
            m_uiManager->GetColorPicker()->SetVisible(true);
        }
        SetForegroundWindow(m_mainWnd->GetHwnd());
    }

    void WindowManager::DeactivateOverlayMode() {
        HideOverlayWindow();
        ShowMainUIAndWindow();
        RecreateGraphicsAndNotify(m_mainWnd->GetHwnd());
    }
}