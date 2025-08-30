// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// WindowManager.cpp: Implementation of centralized window management.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "WindowManager.h"
#include "WindowHelper.h"
#include "Utils.h"

namespace Spectrum {

    WindowManager::WindowManager(HINSTANCE hInstance)
        : m_hInstance(hInstance)
        , m_mode(Mode::Normal) {
    }

    WindowManager::~WindowManager() = default;

    bool WindowManager::Initialize(int width, int height) {
        if (!CreateMainWindow(width, height)) {
            LOG_ERROR("Failed to create main window");
            return false;
        }

        if (!InitializeGraphics()) {
            LOG_ERROR("Failed to initialize graphics");
            return false;
        }

        if (!InitializeColorPicker()) {
            LOG_ERROR("Failed to initialize color picker");
            return false;
        }

        SetupMainWindowCallbacks();
        m_mainWindow->CenterOnScreen();
        return true;
    }

    bool WindowManager::CreateMainWindow(int width, int height) {
        m_mainWindow = std::make_unique<MainWindow>(m_hInstance);
        return m_mainWindow->Initialize(false, width, height);
    }

    bool WindowManager::InitializeGraphics() {
        m_graphics = std::make_unique<GraphicsContext>(m_mainWindow->GetHwnd());
        if (!m_graphics->Initialize()) {
            MessageBoxW(nullptr, L"Failed to initialize graphics", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }

    bool WindowManager::InitializeColorPicker() {
        m_colorPicker = std::make_unique<ColorPicker>(Point(20.0f, 20.0f), 40.0f);
        return m_colorPicker->Initialize(*m_graphics);
    }

    void WindowManager::SetupMainWindowCallbacks() {
        if (!m_mainWindow) return;

        m_mainWindow->SetKeyCallback([this](int key) {
            if (m_keyCallback) m_keyCallback(key);
            });

        m_mainWindow->SetMouseMoveCallback([this](int x, int y) {
            if (m_mouseMoveCallback) m_mouseMoveCallback(x, y);
            });

        m_mainWindow->SetMouseClickCallback([this](int x, int y) {
            if (m_mouseClickCallback) m_mouseClickCallback(x, y);
            });

        m_mainWindow->SetResizeCallback([this](int w, int h) {
            NotifyResize(w, h);
            });

        m_mainWindow->SetCloseCallback([this]() {
            if (m_closeCallback) m_closeCallback();
            });
    }

    void WindowManager::ProcessMessages() {
        if (m_mainWindow)
            m_mainWindow->ProcessMessages();

        if (m_overlayWindow && m_overlayWindow->IsActive())
            m_overlayWindow->ProcessMessages();

        if (m_overlayWindow && m_overlayWindow->ExitRequested() && !m_overlayWindow->IsRunning())
            ExitOverlayMode();
    }

    bool WindowManager::ToggleOverlay() {
        if (m_mode == Mode::Normal) return EnterOverlayMode();
        if (m_mode == Mode::Overlay) return ExitOverlayMode();
        return false;
    }

    bool WindowManager::EnterOverlayMode() {
        if (m_mode != Mode::Normal) return false;
        m_mode = Mode::Transitioning;

        if (!m_overlayWindow)
            m_overlayWindow = std::make_unique<OverlayWindow>(m_hInstance);

        bool ok = m_overlayWindow->Enter(
            *m_mainWindow,
            m_graphics,
            m_colorPicker.get(),
            [this](int key) { if (m_keyCallback) m_keyCallback(key); },
            [this](int w, int h) { NotifyResize(w, h); },
            [this]() { /* overlay requested close, handled in ProcessMessages */ }
        );

        if (!ok) {
            m_mode = Mode::Normal;
            m_overlayWindow.reset();
            LOG_ERROR("Failed to enter overlay mode");
            return false;
        }

        m_mode = Mode::Overlay;
        LOG_INFO("Overlay mode ON");
        return true;
    }

    bool WindowManager::ExitOverlayMode() {
        if (m_mode != Mode::Overlay || !m_overlayWindow) return false;
        m_mode = Mode::Transitioning;

        m_overlayWindow->RequestExit();
        if (!m_overlayWindow->FinalizeExit(*m_mainWindow, m_graphics, m_colorPicker.get())) {
            m_mode = Mode::Overlay;
            LOG_ERROR("Failed to exit overlay mode");
            return false;
        }

        m_overlayWindow.reset();
        m_mode = Mode::Normal;

        SetupMainWindowCallbacks();
        NotifyResize(m_mainWindow->GetWidth(), m_mainWindow->GetHeight());
        SetForegroundWindow(m_mainWindow->GetHwnd());
        LOG_INFO("Overlay mode OFF");
        return true;
    }

    bool WindowManager::IsRunning() const {
        return m_mainWindow && m_mainWindow->IsRunning();
    }

    bool WindowManager::IsActive() const {
        if (m_mode == Mode::Overlay && m_overlayWindow) return m_overlayWindow->IsActive();
        return m_mainWindow && m_mainWindow->IsActive();
    }

    void WindowManager::SetTitle(const std::wstring& title) {
        if (m_mainWindow)
            m_mainWindow->SetTitle(title);
    }

    void WindowManager::CenterWindow() {
        if (m_mainWindow && m_mode == Mode::Normal)
            m_mainWindow->CenterOnScreen();
    }

    void WindowManager::NotifyResize(int width, int height) {
        if (m_graphics)
            m_graphics->Resize(width, height);
        if (m_resizeCallback)
            m_resizeCallback(width, height);
    }

} // namespace Spectrum