// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ControllerCore class, defining the initialization sequence
// for all subsystems and the main application loop logic.
//
// This implementation orchestrates input processing, state collection, and
// rendering through a unified state-driven architecture. Each frame collects
// a complete snapshot of system state (FrameState), which is then propagated
// down through the update pipeline, ensuring deterministic behavior.
//
// Key Implementation Details:
// - State-driven update model: single source of truth per frame
// - Proper frame timing with vsync consideration
// - Device loss handling and resource recreation
// - Early exit patterns for validation and error handling
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include "AudioManager.h"
#include "EventBus.h"
#include "GraphicsContext.h"
#include "InputManager.h"
#include "IRenderer.h"
#include "MainWindow.h"
#include "RendererManager.h"
#include "UIManager.h"
#include "WindowManager.h"
#include <algorithm>
#include <thread>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ControllerCore::ControllerCore(HINSTANCE hInstance) :
        m_hInstance(hInstance),
        m_frameCounter(0)
    {
    }

    ControllerCore::~ControllerCore() noexcept
    {
        Shutdown();
    }

    [[nodiscard]] bool ControllerCore::Initialize()
    {
        if (!InitializeManagers()) return false;

        m_timer.Reset();
        return true;
    }

    void ControllerCore::Run()
    {
        if (!m_windowManager) return;

        m_timer.Reset();
        m_frameCounter = 0;
        MainLoop();
    }

    void ControllerCore::Shutdown()
    {
        m_rendererManager.reset();
        m_audioManager.reset();
        m_inputManager.reset();
        m_windowManager.reset();
        m_eventBus.reset();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // System Event Callbacks
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::OnResize(int width, int height)
    {
        if (!m_rendererManager) return;

        m_rendererManager->OnResize(width, height);
    }

    void ControllerCore::OnCloseRequest()
    {
        if (!m_windowManager) return;
        if (!m_windowManager->IsRunning()) return;

        auto* mainWindow = m_windowManager->GetMainWindow();
        if (!mainWindow) return;

        mainWindow->SetRunning(false);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::SetPrimaryColor(const Color& color)
    {
        if (!m_rendererManager) return;

        auto* currentRenderer = m_rendererManager->GetCurrentRenderer();
        if (!currentRenderer) return;

        currentRenderer->SetPrimaryColor(color);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] RendererManager* ControllerCore::GetRendererManager() const noexcept
    {
        return m_rendererManager.get();
    }

    [[nodiscard]] AudioManager* ControllerCore::GetAudioManager() const noexcept
    {
        return m_audioManager.get();
    }

    [[nodiscard]] WindowManager* ControllerCore::GetWindowManager() const noexcept
    {
        return m_windowManager.get();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool ControllerCore::InitializeManagers()
    {
        m_eventBus = std::make_unique<EventBus>();
        if (!m_eventBus) return false;

        if (!InitializeWindowManager()) return false;
        if (!InitializeAudioManager()) return false;
        if (!InitializeRendererManager()) return false;
        if (!InitializeUIManager()) return false;

        return true;
    }

    [[nodiscard]] bool ControllerCore::InitializeWindowManager()
    {
        m_windowManager = std::make_unique<WindowManager>(
            m_hInstance,
            this,
            m_eventBus.get()
        );

        if (!m_windowManager) return false;
        if (!m_windowManager->Initialize()) return false;

        m_inputManager = std::make_unique<InputManager>();
        if (!m_inputManager) return false;

        return true;
    }

    [[nodiscard]] bool ControllerCore::InitializeAudioManager()
    {
        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());
        if (!m_audioManager) return false;
        if (!m_audioManager->Initialize()) return false;

        return true;
    }

    [[nodiscard]] bool ControllerCore::InitializeRendererManager()
    {
        m_rendererManager = std::make_unique<RendererManager>(
            m_eventBus.get(),
            m_windowManager.get()
        );

        if (!m_rendererManager) return false;
        if (!m_rendererManager->Initialize()) return false;

        auto* graphics = m_windowManager->GetGraphics();
        if (!graphics) return false;

        m_rendererManager->SetCurrentRenderer(
            m_rendererManager->GetCurrentStyle(),
            graphics
        );

        return true;
    }

    [[nodiscard]] bool ControllerCore::InitializeUIManager()
    {
        auto* uiManager = m_windowManager->GetUIManager();
        if (!uiManager) return false;
        if (!uiManager->Initialize()) return false;

        return true;
    }

    void ControllerCore::MainLoop()
    {
        while (m_windowManager->IsRunning())
        {
            m_windowManager->ProcessMessages();

            if (ShouldProcessFrame())
            {
                const FrameState frameState = CollectFrameState();

                ProcessInput();
                Update(frameState);
                Render(frameState);

                ++m_frameCounter;
            }
            else
            {
                ThrottleFrameRate();
            }
        }
    }

    [[nodiscard]] FrameState ControllerCore::CollectFrameState()
    {
        FrameState state;
        state.deltaTime = 1.0f / 60.0f;
        state.frameNumber = m_frameCounter;
        state.isActive = m_windowManager->IsActive();
        state.isOverlayMode = m_windowManager->IsOverlayMode();

        const auto& mouseState = m_windowManager->GetMouseState();
        state.mouse.position = mouseState.position;
        state.mouse.leftButtonDown = mouseState.leftButtonDown;
        state.mouse.rightButtonDown = mouseState.rightButtonDown;
        state.mouse.middleButtonDown = mouseState.middleButtonDown;
        state.mouse.wheelDelta = mouseState.wheelDelta;

        return state;
    }

    void ControllerCore::ProcessInput()
    {
        if (!m_inputManager) return;

        m_inputManager->Update();
        m_actions = m_inputManager->GetActions();

        if (!m_eventBus) return;

        for (const auto& action : m_actions)
            m_eventBus->Publish(action);
    }

    void ControllerCore::Update(const FrameState& frameState)
    {
        if (m_audioManager)
            m_audioManager->Update(frameState.deltaTime);

        auto* uiManager = m_windowManager->GetUIManager();
        if (!uiManager) return;

        uiManager->Update(
            frameState.mouse.position,
            frameState.mouse.leftButtonDown,
            frameState.deltaTime
        );
    }

    void ControllerCore::Render(const FrameState& frameState)
    {
        if (!CanRender(frameState)) return;

        auto* graphics = m_windowManager->GetGraphics();
        if (!graphics) return;

        PrepareFrame(*graphics, frameState);
        RenderSpectrum(*graphics);
        RenderUI(*graphics);
        FinalizeFrame(*graphics);
    }

    [[nodiscard]] bool ControllerCore::CanRender(const FrameState& frameState) const
    {
        if (!frameState.isActive) return false;

        auto* graphics = m_windowManager->GetGraphics();
        if (!graphics) return false;

        auto* renderTarget = graphics->GetRenderTarget();
        if (!renderTarget) return false;

        const D2D1_WINDOW_STATE windowState = renderTarget->CheckWindowState();
        if (windowState & D2D1_WINDOW_STATE_OCCLUDED) return false;

        return true;
    }

    void ControllerCore::PrepareFrame(
        GraphicsContext& graphics,
        const FrameState& frameState
    ) const
    {
        graphics.BeginDraw();

        const Color clearColor = frameState.isOverlayMode
            ? Color::Transparent()
            : Color::FromRGB(13, 13, 26);

        graphics.Clear(clearColor);
    }

    void ControllerCore::RenderSpectrum(GraphicsContext& graphics) const
    {
        if (!m_audioManager) return;
        if (!m_rendererManager) return;

        auto* currentRenderer = m_rendererManager->GetCurrentRenderer();
        if (!currentRenderer) return;

        const SpectrumData spectrum = m_audioManager->GetSpectrum();
        currentRenderer->Render(graphics, spectrum);
    }

    void ControllerCore::RenderUI(GraphicsContext& graphics) const
    {
        if (m_windowManager->IsOverlayMode()) return;

        auto* uiManager = m_windowManager->GetUIManager();
        if (!uiManager) return;

        uiManager->Draw(graphics);
    }

    void ControllerCore::FinalizeFrame(GraphicsContext& graphics)
    {
        const HRESULT hr = graphics.EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            const HWND hwnd = m_windowManager->GetCurrentHwnd();
            if (hwnd)
                m_windowManager->RecreateGraphicsAndNotify(hwnd);
        }
    }

    [[nodiscard]] bool ControllerCore::ShouldProcessFrame() const
    {
        const float elapsed = m_timer.GetElapsedSeconds();
        return elapsed >= kTargetFrameTime;
    }

    void ControllerCore::ThrottleFrameRate() const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

} // namespace Spectrum