#include "ControllerCore.h"

#include "Audio/AudioManager.h"
#include "Common/EventBus.h"
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

    ControllerCore::ControllerCore(HINSTANCE hInstance)
        : m_hInstance(hInstance)
        , m_frameCounter(0)
    {
    }

    ControllerCore::~ControllerCore() noexcept
    {
        Shutdown();
    }

    bool ControllerCore::Initialize()
    {
        if (!InitializeSubsystems())
        {
            return false;
        }

        m_timer.Reset();
        return true;
    }

    void ControllerCore::Run()
    {
        VALIDATE_PTR_OR_RETURN(m_windowManager.get(), "ControllerCore");

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

    void ControllerCore::OnResize(int width, int height)
    {
        if (m_rendererManager)
        {
            m_rendererManager->OnResize(width, height);
        }
    }

    void ControllerCore::OnUIResize(int width, int height)
    {
        auto* uiManager = m_windowManager ? m_windowManager->GetUIManager() : nullptr;
        if (uiManager)
        {
            uiManager->OnResize(width, height);
        }
    }

    void ControllerCore::OnCloseRequest()
    {
        VALIDATE_PTR_OR_RETURN(m_windowManager.get(), "ControllerCore");

        if (auto* mainWindow = m_windowManager->GetMainWindow())
        {
            mainWindow->SetRunning(false);
        }
    }

    // Settings button toggles UI panel visibility for quick access
    void ControllerCore::OnMainWindowClick(const Point& mousePos)
    {
        if (m_settingsButtonRect.Contains(mousePos))
        {
            LOG_INFO("ControllerCore: Settings button toggled.");

            if (m_windowManager)
            {
                if (m_windowManager->IsUIWindowVisible())
                {
                    m_windowManager->HideUIWindow();
                }
                else
                {
                    m_windowManager->ShowUIWindow();
                }
            }
        }
    }

    void ControllerCore::SetPrimaryColor(const Color& color)
    {
        VALIDATE_PTR_OR_RETURN(m_rendererManager.get(), "ControllerCore");

        if (auto* currentRenderer = m_rendererManager->GetCurrentRenderer())
        {
            currentRenderer->SetPrimaryColor(color);
        }
    }

    RendererManager* ControllerCore::GetRendererManager() const noexcept
    {
        return m_rendererManager.get();
    }

    AudioManager* ControllerCore::GetAudioManager() const noexcept
    {
        return m_audioManager.get();
    }

    Platform::WindowManager* ControllerCore::GetWindowManager() const noexcept
    {
        return m_windowManager.get();
    }

    bool ControllerCore::InitializeSubsystems()
    {
        m_eventBus = std::make_unique<EventBus>();
        VALIDATE_PTR_OR_RETURN_FALSE(m_eventBus.get(), "ControllerCore");

        m_windowManager = std::make_unique<Platform::WindowManager>(
            m_hInstance,
            this,
            m_eventBus.get()
        );
        VALIDATE_PTR_OR_RETURN_FALSE(m_windowManager.get(), "ControllerCore");

        if (!m_windowManager->Initialize())
        {
            return false;
        }

        auto keyboard = std::make_unique<Platform::Input::Win32Keyboard>();
        VALIDATE_PTR_OR_RETURN_FALSE(keyboard.get(), "ControllerCore");

        m_inputManager = std::make_unique<Platform::Input::InputManager>(std::move(keyboard));
        VALIDATE_PTR_OR_RETURN_FALSE(m_inputManager.get(), "ControllerCore");

        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());
        VALIDATE_PTR_OR_RETURN_FALSE(m_audioManager.get(), "ControllerCore");

        if (!m_audioManager->Initialize())
        {
            return false;
        }

        m_rendererManager = std::make_unique<RendererManager>(
            m_eventBus.get(),
            m_windowManager.get()
        );
        VALIDATE_PTR_OR_RETURN_FALSE(m_rendererManager.get(), "ControllerCore");

        if (!m_rendererManager->Initialize())
        {
            return false;
        }

        m_rendererManager->SetCurrentRenderer(m_rendererManager->GetCurrentStyle());

        auto* uiManager = m_windowManager->GetUIManager();
        VALIDATE_PTR_OR_RETURN_FALSE(uiManager, "ControllerCore");

        return uiManager->Initialize();
    }

    // Fixed timestep at 60fps keeps animation speed independent from CPU performance
    void ControllerCore::MainLoop()
    {
        MSG msg = {};

        while (m_windowManager->IsRunning())
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (m_windowManager->IsRunning())
            {
                if (ShouldProcessFrame())
                {
                    const FrameState frameState = CollectFrameState();
                    ProcessFrame(frameState);
                    ++m_frameCounter;
                    m_timer.Reset();
                }
                else
                {
                    // Yield CPU when not time for next frame
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    void ControllerCore::ProcessFrame(const FrameState& frameState)
    {
        ProcessInputAndUpdate(frameState);
        RenderVisualization(frameState);
        RenderUI(frameState);
    }

    FrameState ControllerCore::CollectFrameState() const
    {
        FrameState state;
        state.deltaTime = kTargetFrameTime;
        state.frameNumber = m_frameCounter;
        state.isActive = m_windowManager->IsActive();
        state.isOverlayMode = m_windowManager->IsOverlayMode();

        const auto* messageHandler = m_windowManager->GetMessageHandler();
        VALIDATE_PTR_OR_RETURN_VALUE(messageHandler, "ControllerCore", state);

        const auto& handlerMouseState = messageHandler->GetMouseState();
        state.mouse.position = handlerMouseState.position;
        state.mouse.leftButtonDown = handlerMouseState.leftButtonDown;
        state.mouse.rightButtonDown = handlerMouseState.rightButtonDown;
        state.mouse.middleButtonDown = handlerMouseState.middleButtonDown;
        state.mouse.wheelDelta = handlerMouseState.wheelDelta;

        return state;
    }

    void ControllerCore::ProcessInputAndUpdate(const FrameState& frameState)
    {
        VALIDATE_PTR_OR_RETURN(m_inputManager.get(), "ControllerCore");
        VALIDATE_PTR_OR_RETURN(m_eventBus.get(), "ControllerCore");

        m_inputManager->Update();
        m_actions = m_inputManager->GetActions();

        for (const auto& action : m_actions)
        {
            m_eventBus->Publish(action);
        }

        if (m_audioManager)
        {
            m_audioManager->Update(frameState.deltaTime);
        }
    }

    // D2DERR_RECREATE_TARGET returned when GPU device removed or driver updated
    void ControllerCore::RenderVisualization(const FrameState& frameState)
    {
        if (!CanRenderVisualization(frameState))
        {
            return;
        }

        auto* engine = m_windowManager->GetVisualizationEngine();
        VALIDATE_PTR_OR_RETURN(engine, "ControllerCore");

        if (!engine->BeginDraw())
        {
            LOG_ERROR("ControllerCore: Failed to begin visualization drawing.");
            return;
        }

        // Transparent background needed for overlay mode to show through to desktop
        const Color clearColor = frameState.isOverlayMode
            ? Color::Transparent()
            : Color::FromRGB(13, 13, 26);
        engine->Clear(clearColor);

        VALIDATE_PTR_OR_RETURN(m_audioManager.get(), "ControllerCore");

        auto* currentRenderer = m_rendererManager->GetCurrentRenderer();
        VALIDATE_PTR_OR_RETURN(currentRenderer, "ControllerCore");

        currentRenderer->Render(engine->GetCanvas(), m_audioManager->GetSpectrum());

        RenderSettingsButton(frameState);

        if (engine->EndDraw() == D2DERR_RECREATE_TARGET)
        {
            HandleVisualizationDeviceLoss();
        }
    }

    void ControllerCore::RenderUI(const FrameState& frameState)
    {
        (void)frameState;

        if (!CanRenderUI())
        {
            return;
        }

        auto* uiManager = m_windowManager->GetUIManager();
        VALIDATE_PTR_OR_RETURN(uiManager, "ControllerCore");

        auto* uiEngine = m_windowManager->GetUIEngine();
        VALIDATE_PTR_OR_RETURN(uiEngine, "ControllerCore");

        uiEngine->ClearD3D11(Color::FromRGB(30, 30, 40));
        uiManager->BeginFrame();
        uiManager->Render();
        uiManager->EndFrame();
        uiEngine->Present();
    }

    void ControllerCore::RenderSettingsButton(const FrameState& frameState)
    {
        auto* engine = m_windowManager->GetVisualizationEngine();
        if (!engine)
        {
            return;
        }

        const float buttonSize = 30.0f;
        const float padding = 10.0f;
        const float x = static_cast<float>(engine->GetWidth()) - buttonSize - padding;
        const float y = padding;

        m_settingsButtonRect = Rect(x, y, buttonSize, buttonSize);
        bool isHovered = m_settingsButtonRect.Contains(frameState.mouse.position);

        TextStyle style = TextStyle::Default()
            .WithFont(L"Segoe UI Symbol")
            .WithSize(24.0f)
            .WithAlign(TextAlign::Center)
            .WithParagraphAlign(ParagraphAlign::Center)
            .WithColor(isHovered
                ? Color(1.0f, 1.0f, 1.0f, 1.0f)
                : Color(1.0f, 1.0f, 1.0f, 0.5f));

        auto& canvas = engine->GetCanvas();
        canvas.DrawText(L"⚙", m_settingsButtonRect, style);
    }

    // Overlay mode renders even when inactive because user wants visualization visible always
    bool ControllerCore::CanRenderVisualization(const FrameState& frameState) const
    {
        if (frameState.isOverlayMode)
        {
            return true;
        }

        return frameState.isActive;
    }

    bool ControllerCore::CanRenderUI() const
    {
        return m_windowManager && m_windowManager->IsUIWindowVisible();
    }

    bool ControllerCore::ShouldProcessFrame() const
    {
        return m_timer.GetElapsedSeconds() >= kTargetFrameTime;
    }

    void ControllerCore::HandleVisualizationDeviceLoss()
    {
        VALIDATE_PTR_OR_RETURN(m_windowManager.get(), "ControllerCore");

        auto* engine = m_windowManager->GetVisualizationEngine();
        VALIDATE_PTR_OR_RETURN(engine, "ControllerCore");

        if (!m_windowManager->HandleVisualizationResize(
            engine->GetWidth(),
            engine->GetHeight(),
            true))
        {
            LOG_ERROR("ControllerCore: Failed to recreate visualization graphics after device loss.");
        }
    }

    void ControllerCore::HandleUIDeviceLoss()
    {
        VALIDATE_PTR_OR_RETURN(m_windowManager.get(), "ControllerCore");

        auto* engine = m_windowManager->GetUIEngine();
        VALIDATE_PTR_OR_RETURN(engine, "ControllerCore");

        if (!m_windowManager->HandleUIResize(
            engine->GetWidth(),
            engine->GetHeight(),
            true))
        {
            LOG_ERROR("ControllerCore: Failed to recreate UI graphics after device loss.");
        }
    }

}  // namespace Spectrum