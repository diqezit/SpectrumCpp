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

#include <thread>

namespace Spectrum {

    namespace {
        constexpr Color kUIBackgroundColor = Color::FromRGB(30, 30, 40);
    }

    ControllerCore::ControllerCore(HINSTANCE hInstance)
        : m_hInstance(hInstance) {
    }

    ControllerCore::~ControllerCore() noexcept {
        Shutdown();
    }

    bool ControllerCore::Initialize() {
        if (!InitializeSubsystems()) {
            return false;
        }
        m_timer.Reset();
        return true;
    }

    void ControllerCore::Run() {
        VALIDATE_PTR_OR_RETURN(m_windowManager.get(), "ControllerCore");

        m_timer.Reset();
        m_frameCounter = 0;
        MainLoop();
    }

    void ControllerCore::Shutdown() {
        if (m_audioManager) {
            m_audioManager->Shutdown();
        }

        m_rendererManager.reset();
        m_audioManager.reset();
        m_inputManager.reset();
        m_windowManager.reset();
        m_eventBus.reset();
    }

    void ControllerCore::OnResize(int width, int height) {
        if (m_rendererManager) {
            m_rendererManager->OnResize(width, height);
        }
    }

    void ControllerCore::OnUIResize(int width, int height) {
        if (auto* uiManager = m_windowManager ? m_windowManager->GetUIManager() : nullptr) {
            uiManager->OnResize(width, height);
        }
    }

    void ControllerCore::OnCloseRequest() {
        if (m_windowManager) {
            if (auto* mainWindow = m_windowManager->GetMainWindow()) {
                mainWindow->SetRunning(false);
            }
        }
    }

    void ControllerCore::OnMainWindowClick(const Point& mousePos) {
        if (!m_windowManager || !m_settingsButtonRect.Contains(mousePos)) {
            return;
        }

        LOG_INFO("ControllerCore: Settings button toggled.");

        if (m_windowManager->IsUIWindowVisible()) {
            m_windowManager->HideUIWindow();
        }
        else {
            m_windowManager->ShowUIWindow();
        }
    }

    void ControllerCore::SetPrimaryColor(const Color& color) {
        if (m_rendererManager) {
            if (auto* renderer = m_rendererManager->GetCurrentRenderer()) {
                renderer->SetPrimaryColor(color);
            }
        }
    }

    bool ControllerCore::InitializeSubsystems() {
        m_eventBus = std::make_unique<EventBus>();
        VALIDATE_PTR_OR_RETURN_FALSE(m_eventBus.get(), "ControllerCore");

        m_windowManager = std::make_unique<Platform::WindowManager>(
            m_hInstance, this, m_eventBus.get()
        );
        VALIDATE_PTR_OR_RETURN_FALSE(m_windowManager.get(), "ControllerCore");

        if (!m_windowManager->Initialize()) {
            return false;
        }

        auto keyboard = std::make_unique<Platform::Input::Win32Keyboard>();
        VALIDATE_PTR_OR_RETURN_FALSE(keyboard.get(), "ControllerCore");

        m_inputManager = std::make_unique<Platform::Input::InputManager>(std::move(keyboard));
        VALIDATE_PTR_OR_RETURN_FALSE(m_inputManager.get(), "ControllerCore");

        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());
        VALIDATE_PTR_OR_RETURN_FALSE(m_audioManager.get(), "ControllerCore");

        if (!m_audioManager->Initialize()) {
            return false;
        }

        m_rendererManager = std::make_unique<RendererManager>(
            m_eventBus.get(), m_windowManager.get()
        );
        VALIDATE_PTR_OR_RETURN_FALSE(m_rendererManager.get(), "ControllerCore");

        if (!m_rendererManager->Initialize()) {
            return false;
        }

        m_rendererManager->SetCurrentRenderer(m_rendererManager->GetCurrentStyle());

        return true;
    }

    void ControllerCore::MainLoop() {
        MSG msg = {};

        while (m_windowManager->IsRunning()) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!m_windowManager->IsRunning()) {
                break;
            }

            if (ShouldProcessFrame()) {
                ProcessFrame();
                ++m_frameCounter;
                m_timer.Reset();
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void ControllerCore::ProcessFrame() {
        const FrameState frameState = CollectFrameState();

        ProcessInputAndUpdate(frameState.deltaTime);

        if (frameState.isOverlayMode || frameState.isActive) {
            RenderVisualization(frameState);
        }

        if (m_windowManager && m_windowManager->IsUIWindowVisible()) {
            RenderUI();
        }
    }

    FrameState ControllerCore::CollectFrameState() const {
        FrameState state{};
        state.deltaTime = FRAME_TIME;
        state.frameNumber = m_frameCounter;
        state.isActive = m_windowManager->IsActive();
        state.isOverlayMode = m_windowManager->IsOverlayMode();

        if (const auto* messageHandler = m_windowManager->GetMessageHandler()) {
            const auto& handlerMouseState = messageHandler->GetMouseState();
            state.mouse.position = handlerMouseState.position;
            state.mouse.leftButtonDown = handlerMouseState.leftButtonDown;
            state.mouse.rightButtonDown = handlerMouseState.rightButtonDown;
            state.mouse.middleButtonDown = handlerMouseState.middleButtonDown;
            state.mouse.wheelDelta = handlerMouseState.wheelDelta;
        }

        return state;
    }

    void ControllerCore::ProcessInputAndUpdate(float deltaTime) {
        if (!m_inputManager || !m_eventBus) {
            return;
        }

        m_inputManager->Update();

        for (const auto& action : m_inputManager->GetActions()) {
            m_eventBus->Publish(action);
        }

        if (m_audioManager) {
            m_audioManager->Update(deltaTime);
        }
    }

    void ControllerCore::RenderVisualization(const FrameState& frameState) {
        auto* engine = m_windowManager->GetVisualizationEngine();
        if (!engine || !engine->BeginDraw()) {
            LOG_ERROR("ControllerCore: Failed to begin visualization drawing.");
            return;
        }

        const Color clearColor = frameState.isOverlayMode
            ? Color::Transparent()
            : Color::FromRGB(13, 13, 26);
        engine->Clear(clearColor);

        if (m_audioManager && m_rendererManager) {
            if (auto* renderer = m_rendererManager->GetCurrentRenderer()) {
                renderer->Render(engine->GetCanvas(), m_audioManager->GetSpectrum());
            }
        }

        RenderSettingsButton(frameState);

        if (engine->EndDraw() == D2DERR_RECREATE_TARGET) {
            HandleDeviceLoss(engine);
        }
    }

    void ControllerCore::RenderUI() {
        auto* uiManager = m_windowManager->GetUIManager();
        auto* uiEngine = m_windowManager->GetUIEngine();

        if (!uiManager || !uiEngine) {
            return;
        }

        uiEngine->ClearD3D11(kUIBackgroundColor);
        uiManager->BeginFrame();
        uiManager->Render();
        uiManager->EndFrame();
        uiEngine->Present();
    }

    void ControllerCore::RenderSettingsButton(const FrameState& frameState) {
        auto* engine = m_windowManager->GetVisualizationEngine();
        if (!engine) {
            return;
        }

        constexpr float BUTTON_SIZE = 30.0f;
        constexpr float PADDING = 10.0f;

        const float x = static_cast<float>(engine->GetWidth()) - BUTTON_SIZE - PADDING;
        m_settingsButtonRect = Rect(x, PADDING, BUTTON_SIZE, BUTTON_SIZE);

        const bool isHovered = m_settingsButtonRect.Contains(frameState.mouse.position);

        TextStyle style = TextStyle::Default()
            .WithFont(L"Segoe UI Symbol")
            .WithSize(24.0f)
            .WithAlign(TextAlign::Center)
            .WithParagraphAlign(ParagraphAlign::Center)
            .WithColor(isHovered
                ? Color(1.0f, 1.0f, 1.0f, 1.0f)
                : Color(1.0f, 1.0f, 1.0f, 0.5f));

        engine->GetCanvas().DrawText(L"⚙", m_settingsButtonRect, style);
    }

    bool ControllerCore::ShouldProcessFrame() const {
        return m_timer.GetElapsedSeconds() >= FRAME_TIME;
    }

    void ControllerCore::HandleDeviceLoss(RenderEngine* engine) {
        if (!m_windowManager || !engine) {
            return;
        }

        const bool isUI = (engine == m_windowManager->GetUIEngine());
        const bool success = isUI
            ? m_windowManager->HandleUIResize(engine->GetWidth(), engine->GetHeight(), true)
            : m_windowManager->HandleVisualizationResize(engine->GetWidth(), engine->GetHeight(), true);

        if (!success) {
            if (isUI) {
                LOG_ERROR("ControllerCore: Failed to recreate UI graphics after device loss.");
            }
            else {
                LOG_ERROR("ControllerCore: Failed to recreate visualization graphics after device loss.");
            }
        }
    }

}