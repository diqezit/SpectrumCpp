// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the ControllerCore class
// It initializes and coordinates all the application's managers and
// contains the main application loop logic (Input -> Update -> Render).
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include "IRenderer.h"
#include "WindowManager.h"
#include "AudioManager.h"
#include "RendererManager.h"
#include "InputManager.h"
#include "WindowHelper.h"
#include "GraphicsContext.h"
#include "UIManager.h"
#include "EventBus.h"

namespace Spectrum {

    ControllerCore::ControllerCore(HINSTANCE hInstance)
        : m_hInstance(hInstance) {
    }

    ControllerCore::~ControllerCore() = default;

    bool ControllerCore::Initialize() {
        if (!InitializeManagers())
            return false;
        PrintWelcomeMessage();
        return true;
    }

    void ControllerCore::Run() {
        m_timer.Reset();
        MainLoop();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    bool ControllerCore::InitializeEventBus() {
        m_eventBus = std::make_unique<EventBus>();
        return true;
    }

    bool ControllerCore::InitializeWindowManager() {
        m_windowManager = std::make_unique<WindowManager>(m_hInstance, this, m_eventBus.get());
        return m_windowManager->Initialize();
    }

    bool ControllerCore::InitializeInputManager() {
        m_inputManager = std::make_unique<InputManager>();
        return true;
    }

    bool ControllerCore::InitializeAudioManager() {
        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());
        return m_audioManager->Initialize();
    }

    bool ControllerCore::InitializeRendererManager() {
        m_rendererManager = std::make_unique<RendererManager>(m_eventBus.get(), m_windowManager.get());
        return m_rendererManager->Initialize();
    }

    // Initialization order is critical for dependencies
    bool ControllerCore::InitializeManagers() {
        if (!InitializeEventBus() ||
            !InitializeWindowManager() ||
            !InitializeInputManager() ||
            !InitializeAudioManager() ||
            !InitializeRendererManager())
            return false;

        // Show a visualizer on screen immediately after launch
        m_rendererManager->SetCurrentRenderer(
            m_rendererManager->GetCurrentStyle(), m_windowManager->GetGraphics()
        );

        return true;
    }

    // Show controls in console to guide new users
    void ControllerCore::PrintWelcomeMessage() {
        LOG_INFO("========================================");
        LOG_INFO("     Spectrum Visualizer C++");
        LOG_INFO("========================================");
        LOG_INFO("Controls:");
        LOG_INFO("  SPACE - Toggle audio capture");
        LOG_INFO("  A     - Toggle animation (test mode)");
        LOG_INFO("  R     - Switch renderer");
        LOG_INFO("  Q     - Change render quality");
        LOG_INFO("  O     - Toggle Overlay Mode");
        LOG_INFO("  S     - Switch Spectrum Scale");
        LOG_INFO("  UP/DOWN Arrow  - Change Amplification");
        LOG_INFO("  LEFT/RIGHT Arrow - Change FFT Window");
        LOG_INFO("  -/+ Keys       - Change Bar Count");
        LOG_INFO("  ESC   - Exit");
        LOG_INFO("========================================");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Main Application Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    void ControllerCore::MainLoop() {
        while (m_windowManager->IsRunning()) {
            m_windowManager->ProcessMessages();

            // Fixed timestep gives smooth animation independent of frame rate
            float dt = m_timer.GetElapsedSeconds();
            if (dt >= FRAME_TIME) {
                m_timer.Reset();
                ProcessInput();
                Update(dt);
                Render();
            }
            else {
                // Yield CPU time to avoid 100% usage when idle
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void ControllerCore::ProcessInput() {
        m_inputManager->Update();
        m_actions = m_inputManager->GetActions();
        // Broadcast user actions for any system to handle
        for (const auto& action : m_actions)
            m_eventBus->Publish(action);
    }

    void ControllerCore::Update(float deltaTime) {
        m_audioManager->Update(deltaTime);
    }

    // Avoids drawing when window is minimized or fully covered to save GPU
    bool ControllerCore::CanRender() const {
        auto* graphics = m_windowManager->GetGraphics();
        if (!graphics || !m_windowManager->IsActive())
            return false;

        if (auto* rt = graphics->GetRenderTarget())
            if (rt->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)
                return false;

        return true;
    }

    void ControllerCore::PrepareFrame(GraphicsContext& graphics) {
        graphics.BeginDraw();

        const Color clearColor = m_windowManager->IsOverlayMode()
            ? Color::Transparent() // Overlay mode requires transparent background
            : Color::FromRGB(13, 13, 26);
        graphics.Clear(clearColor);
    }

    void ControllerCore::RenderSpectrum(GraphicsContext& graphics) {
        SpectrumData spectrum = m_audioManager->GetSpectrum();
        if (m_rendererManager->GetCurrentRenderer())
            m_rendererManager->GetCurrentRenderer()->Render(graphics, spectrum);
    }

    // UI is disabled in overlay mode so user can see through the window
    void ControllerCore::RenderUI(GraphicsContext& graphics) {
        if (auto* uiManager = m_windowManager->GetUIManager())
            if (!m_windowManager->IsOverlayMode())
                uiManager->Draw(graphics);
    }

    // Handle device loss to recover from driver updates or GPU resets
    void ControllerCore::FinalizeFrame(GraphicsContext& graphics) {
        HRESULT hr = graphics.EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            HWND hwnd = m_windowManager->GetCurrentHwnd();
            if (hwnd)
                m_windowManager->RecreateGraphicsAndNotify(hwnd);
        }
    }

    void ControllerCore::Render() {
        if (!CanRender())
            return;

        auto* graphics = m_windowManager->GetGraphics();

        PrepareFrame(*graphics);
        RenderSpectrum(*graphics);
        RenderUI(*graphics);
        FinalizeFrame(*graphics);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Callbacks & Event Handlers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    void ControllerCore::OnResize(int width, int height) {
        if (m_windowManager) {
            auto* graphics = m_windowManager->GetGraphics();
            if (graphics)
                graphics->Resize(width, height);
        }
        if (m_rendererManager)
            m_rendererManager->OnResize(width, height);
    }

    // Apply color changes from UI to the active visualizer
    void ControllerCore::SetPrimaryColor(const Color& color) {
        if (m_rendererManager && m_rendererManager->GetCurrentRenderer())
            m_rendererManager->GetCurrentRenderer()->SetPrimaryColor(color);
    }

    // Signal a graceful exit instead of forcing termination
    void ControllerCore::OnClose() {
        if (m_windowManager && m_windowManager->IsRunning())
            if (auto* wnd = m_windowManager->GetMainWindow())
                wnd->SetRunning(false);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Win32 Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    // Allow dragging frameless window like a title bar in overlay mode
    // Suppress default background erase to prevent flicker during our draw
    LRESULT ControllerCore::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CLOSE:
            OnClose();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (wParam != SIZE_MINIMIZED)
                OnResize(width, height);
            return 0;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
            return HandleMouseMessage(msg, lParam);
        case WM_NCHITTEST:
            if (m_windowManager->IsOverlayMode())
                return HTCAPTION;
            break;
        case WM_ERASEBKGND:
            return 1;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT ControllerCore::HandleMouseMessage(UINT msg, LPARAM lParam) {
        int x, y;
        WindowUtils::ExtractMouse(lParam, x, y);

        bool needsRedraw = false;
        if (auto* uiManager = m_windowManager->GetUIManager())
            needsRedraw = uiManager->HandleMouseMessage(msg, x, y);

        // Request a redraw only if a UI element's state changed
        if (needsRedraw) {
            HWND hwnd = m_windowManager->GetCurrentHwnd();
            if (hwnd)
                InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
} // namespace Spectrum