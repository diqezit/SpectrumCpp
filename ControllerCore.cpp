// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the ControllerCore class. It initializes and
// coordinates all the application's managers and contains the main
// application loop logic (Input -> Update -> Render).
//
// Implements the ControllerCore by defining the initialization sequence for
// all subsystems and the logic for the main application loop. It orchestrates
// input processing, state updates, and rendering, delegating specific tasks
// to the appropriate managers.
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

namespace Spectrum {

    ControllerCore::ControllerCore(HINSTANCE hInstance) :
        m_hInstance(hInstance)
    {
    }

    ControllerCore::~ControllerCore() noexcept = default;

    [[nodiscard]] bool ControllerCore::Initialize()
    {
        if (!InitializeManagers()) return false;
        return true;
    }

    void ControllerCore::Run()
    {
        m_timer.Reset();
        MainLoop();
    }

    void ControllerCore::OnResize(int width, int height)
    {
        if (m_rendererManager)
            m_rendererManager->OnResize(width, height);
    }

    void ControllerCore::OnCloseRequest()
    {
        if (m_windowManager && m_windowManager->IsRunning())
            if (auto* wnd = m_windowManager->GetMainWindow())
                wnd->SetRunning(false);
    }

    void ControllerCore::SetPrimaryColor(const Color& color)
    {
        if (m_rendererManager && m_rendererManager->GetCurrentRenderer())
            m_rendererManager->GetCurrentRenderer()->SetPrimaryColor(color);
    }

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

    [[nodiscard]] bool ControllerCore::InitializeManagers()
    {
        m_eventBus = std::make_unique<EventBus>();
        if (!m_eventBus) return false;

        m_windowManager = std::make_unique<WindowManager>(m_hInstance, this, m_eventBus.get());
        if (!m_windowManager || !m_windowManager->Initialize()) return false;

        m_inputManager = std::make_unique<InputManager>();
        if (!m_inputManager) return false;

        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());
        if (!m_audioManager || !m_audioManager->Initialize()) return false;

        m_rendererManager = std::make_unique<RendererManager>(m_eventBus.get(), m_windowManager.get());
        if (!m_rendererManager || !m_rendererManager->Initialize()) return false;

        if (m_windowManager->GetUIManager())
            if (!m_windowManager->GetUIManager()->Initialize())
                return false;

        m_rendererManager->SetCurrentRenderer(
            m_rendererManager->GetCurrentStyle(),
            m_windowManager->GetGraphics()
        );

        return true;
    }

    void ControllerCore::MainLoop()
    {
        while (m_windowManager->IsRunning())
        {
            m_windowManager->ProcessMessages();

            const float dt = m_timer.GetElapsedSeconds();
            if (dt >= FRAME_TIME)
            {
                m_timer.Reset();

                const auto& mouseState = m_windowManager->GetMouseState();
                if (auto* uiManager = m_windowManager->GetUIManager())
                {
                    uiManager->Update(mouseState.position, mouseState.leftButtonDown, dt);
                }

                ProcessInput();
                Update(dt);
                Render();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void ControllerCore::ProcessInput()
    {
        m_inputManager->Update();
        m_actions = m_inputManager->GetActions();

        for (const auto& action : m_actions)
            m_eventBus->Publish(action);
    }

    void ControllerCore::Update(float deltaTime)
    {
        m_audioManager->Update(deltaTime);
    }

    void ControllerCore::Render()
    {
        if (!CanRender()) return;

        auto* graphics = m_windowManager->GetGraphics();

        PrepareFrame(*graphics);
        RenderSpectrum(*graphics);
        RenderUI(*graphics);
        FinalizeFrame(*graphics);
    }

    [[nodiscard]] bool ControllerCore::CanRender() const
    {
        auto* graphics = m_windowManager->GetGraphics();
        if (!graphics || !m_windowManager->IsActive()) return false;

        if (auto* rt = graphics->GetRenderTarget())
            if (rt->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)
                return false;

        return true;
    }

    void ControllerCore::PrepareFrame(GraphicsContext& graphics) const
    {
        graphics.BeginDraw();

        const Color clearColor = m_windowManager->IsOverlayMode()
            ? Color::Transparent()
            : Color::FromRGB(13, 13, 26);
        graphics.Clear(clearColor);
    }

    void ControllerCore::RenderSpectrum(GraphicsContext& graphics) const
    {
        const SpectrumData spectrum = m_audioManager->GetSpectrum();
        if (m_rendererManager->GetCurrentRenderer())
            m_rendererManager->GetCurrentRenderer()->Render(graphics, spectrum);
    }

    void ControllerCore::RenderUI(GraphicsContext& graphics) const
    {
        if (auto* uiManager = m_windowManager->GetUIManager())
            if (!m_windowManager->IsOverlayMode())
                uiManager->Draw(graphics);
    }

    void ControllerCore::FinalizeFrame(GraphicsContext& graphics) const
    {
        const HRESULT hr = graphics.EndDraw();
        if (hr == D2DERR_RECREATE_TARGET)
        {
            HWND hwnd = m_windowManager->GetCurrentHwnd();
            if (hwnd)
                m_windowManager->RecreateGraphicsAndNotify(hwnd);
        }
    }

}