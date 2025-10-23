// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ControllerCore class, defining the initialization sequence
// for all subsystems and the main application loop logic.
//
// This implementation orchestrates input processing, state collection, and
// rendering through a unified state-driven architecture. Each frame collects
// a complete snapshot of system state (FrameState), which is then propagated
// down through the update pipeline, ensuring deterministic behavior.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include "Audio/AudioManager.h"
#include "Common/EventBus.h"
#include "Graphics/Api/Core/RenderEngine.h"
#include "Graphics/Api/Canvas.h"
#include "Platform/Input/InputManager.h"
#include "Graphics/IRenderer.h"
#include "Platform/MainWindow.h"
#include "Platform/MessageHandler.h"
#include "Graphics/RendererManager.h"
#include "UI/Core/UIManager.h"
#include "Platform/WindowManager.h"
#include "Platform/Input/Win32Keyboard.h"
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
        if (m_rendererManager) m_rendererManager->OnResize(width, height);
    }

    void ControllerCore::OnCloseRequest()
    {
        if (!m_windowManager) return;
        if (!m_windowManager->IsRunning()) return;

        if (auto* mainWindow = m_windowManager->GetMainWindow())
        {
            mainWindow->SetRunning(false);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::SetPrimaryColor(const Color& color)
    {
        if (!m_rendererManager) return;
        if (auto* currentRenderer = m_rendererManager->GetCurrentRenderer())
        {
            currentRenderer->SetPrimaryColor(color);
        }
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

    [[nodiscard]] Platform::WindowManager* ControllerCore::GetWindowManager() const noexcept
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
        m_windowManager = std::make_unique<Platform::WindowManager>(
            m_hInstance, this, m_eventBus.get()
        );
        if (!m_windowManager) return false;
        if (!m_windowManager->Initialize()) return false;

        auto keyboard = std::make_unique<Platform::Input::Win32Keyboard>();
        m_inputManager = std::make_unique<Platform::Input::InputManager>(std::move(keyboard));
        if (!m_inputManager) return false;

        return true;
    }

    [[nodiscard]] bool ControllerCore::InitializeAudioManager()
    {
        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());
        return m_audioManager && m_audioManager->Initialize();
    }

    [[nodiscard]] bool ControllerCore::InitializeRendererManager()
    {
        m_rendererManager = std::make_unique<RendererManager>(
            m_eventBus.get(), m_windowManager.get()
        );
        if (!m_rendererManager || !m_rendererManager->Initialize()) return false;

        m_rendererManager->SetCurrentRenderer(m_rendererManager->GetCurrentStyle());
        return true;
    }

    [[nodiscard]] bool ControllerCore::InitializeUIManager()
    {
        if (auto* uiManager = m_windowManager->GetUIManager())
        {
            return uiManager->Initialize();
        }
        return false;
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

        if (const auto* messageHandler = m_windowManager->GetMessageHandler())
        {
            const auto& mouseState = messageHandler->GetMouseState();
            state.mouse.position = mouseState.position;
            state.mouse.leftButtonDown = mouseState.leftButtonDown;
            state.mouse.rightButtonDown = mouseState.rightButtonDown;
            state.mouse.middleButtonDown = mouseState.middleButtonDown;
            state.mouse.wheelDelta = mouseState.wheelDelta;
        }

        return state;
    }

    void ControllerCore::ProcessInput()
    {
        if (!m_inputManager || !m_eventBus) return;

        m_inputManager->Update();
        m_actions = m_inputManager->GetActions();

        for (const auto& action : m_actions)
        {
            m_eventBus->Publish(action);
        }
    }

    void ControllerCore::Update(const FrameState& frameState)
    {
        if (m_audioManager) m_audioManager->Update(frameState.deltaTime);
        if (auto* uiManager = m_windowManager->GetUIManager())
        {
            uiManager->Update(
                frameState.mouse.position,
                frameState.mouse.leftButtonDown,
                frameState.deltaTime
            );
        }
    }

    void ControllerCore::Render(const FrameState& frameState)
    {
        if (!CanRender(frameState)) return;

        PrepareFrame(frameState);
        RenderSpectrum();
        RenderUI();
        FinalizeFrame();
    }

    [[nodiscard]] bool ControllerCore::CanRender(const FrameState& frameState) const
    {
        if (frameState.isOverlayMode)
        {
            if (auto* engine = m_windowManager->GetRenderEngine())
            {
                if (auto* rt = engine->GetRenderTarget())
                {
                    return (rt->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED) == 0;
                }
            }
            return false;
        }
        return frameState.isActive;
    }

    void ControllerCore::PrepareFrame(const FrameState& frameState) const
    {
        auto* engine = m_windowManager->GetRenderEngine();
        if (!engine) return;

        engine->BeginDraw();
        const Color clearColor = frameState.isOverlayMode
            ? Color::Transparent()
            : Color::FromRGB(13, 13, 26);
        engine->Clear(clearColor);
    }

    void ControllerCore::RenderSpectrum() const
    {
        if (!m_audioManager || !m_rendererManager) return;

        auto* engine = m_windowManager->GetRenderEngine();
        if (!engine) return;

        if (auto* currentRenderer = m_rendererManager->GetCurrentRenderer())
        {
            const SpectrumData spectrum = m_audioManager->GetSpectrum();
            currentRenderer->Render(engine->GetCanvas(), spectrum);
        }
    }

    void ControllerCore::RenderUI() const
    {
        if (m_windowManager->IsOverlayMode()) return;

        auto* engine = m_windowManager->GetRenderEngine();
        if (!engine) return;

        if (auto* uiManager = m_windowManager->GetUIManager())
        {
            uiManager->Draw(engine->GetCanvas());
        }
    }

    void ControllerCore::FinalizeFrame()
    {
        auto* engine = m_windowManager->GetRenderEngine();
        if (!engine) return;

        if (engine->EndDraw() == D2DERR_RECREATE_TARGET)
        {
            if (const HWND hwnd = m_windowManager->GetCurrentHwnd())
            {
                if (!m_windowManager->RecreateGraphicsAndNotify(hwnd))
                {
                    LOG_ERROR("Failed to recreate graphics context after device loss");
                }
            }
        }
    }

    [[nodiscard]] bool ControllerCore::ShouldProcessFrame() const
    {
        return m_timer.GetElapsedSeconds() >= kTargetFrameTime;
    }

    void ControllerCore::ThrottleFrameRate() const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

} // namespace Spectrum