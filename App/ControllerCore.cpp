// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the main application orchestrator
//
// Implementation details:
// - Initializes subsystems sequentially to guarantee dependencies are met
// - Uses immutable FrameState to prevent mid-frame state corruption
// - Runs a fixed-timestep loop for predictable animation and updates
// - Separates each frame into Input Update Render phases for clarity
// - Recovers from graphics device loss to prevent application crashes
//
// Initialization sequence:
// - Follows a strict startup order to resolve dependencies correctly
// - EventBus is created first because all subsystems depend on it
// - WindowManager is next to provide a valid HWND for other systems
// - RendererManager and UIManager are last as they need a full graphics context
//
// Frame pipeline:
// - Gathers a single immutable state snapshot at the start of each frame
// - Decouples input from game logic by publishing actions to the EventBus
// - Updates all subsystem logic based on time and input before rendering
// - Renders the final state only if the window is active and not occluded
// - Checks for graphics errors after each frame to trigger recovery if needed
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include "Audio/AudioManager.h"
#include "Common/EventBus.h"
#include "Graphics/Api/Core/RenderEngine.h"
#include "Graphics/Api/Canvas.h"
#include "Graphics/API/Helpers/D2D/RenderTargetHelpers.h"
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

    ControllerCore::ControllerCore(HINSTANCE hInstance)
        : m_hInstance(hInstance)
        , m_frameCounter(0)
    {
    }

    ControllerCore::~ControllerCore() noexcept
    {
        Shutdown();
    }

    [[nodiscard]] bool ControllerCore::Initialize()
    {
        if (!InitializeSubsystems()) {
            return false;
        }

        ResetFrameTimer();
        return true;
    }

    void ControllerCore::Run()
    {
        if (!ValidateWindowManager()) {
            return;
        }

        ResetFrameTimer();
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
        if (m_rendererManager) {
            m_rendererManager->OnResize(width, height);
        }
    }

    void ControllerCore::OnCloseRequest()
    {
        if (!ValidateWindowManager()) {
            return;
        }

        if (auto* mainWindow = m_windowManager->GetMainWindow()) {
            mainWindow->SetRunning(false);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::SetPrimaryColor(const Color& color)
    {
        if (!ValidateRendererManager()) {
            return;
        }

        if (auto* currentRenderer = m_rendererManager->GetCurrentRenderer()) {
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
    // Subsystem Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool ControllerCore::InitializeSubsystems()
    {
        if (!InitializeEventBus()) return false;
        if (!InitializeWindowManager()) return false;
        if (!InitializeInputManager()) return false;
        if (!InitializeAudioManager()) return false;
        if (!InitializeRendererManager()) return false;
        if (!InitializeUIManager()) return false;

        return true;
    }

    bool ControllerCore::InitializeEventBus()
    {
        m_eventBus = std::make_unique<EventBus>();
        return m_eventBus != nullptr;
    }

    bool ControllerCore::InitializeWindowManager()
    {
        m_windowManager = std::make_unique<Platform::WindowManager>(
            m_hInstance,
            this,
            m_eventBus.get()
        );

        if (!m_windowManager) {
            return false;
        }

        return m_windowManager->Initialize();
    }

    bool ControllerCore::InitializeInputManager()
    {
        auto keyboard = std::make_unique<Platform::Input::Win32Keyboard>();
        if (!keyboard) {
            return false;
        }

        m_inputManager = std::make_unique<Platform::Input::InputManager>(
            std::move(keyboard)
        );

        return m_inputManager != nullptr;
    }

    bool ControllerCore::InitializeAudioManager()
    {
        m_audioManager = std::make_unique<AudioManager>(m_eventBus.get());

        if (!m_audioManager) {
            return false;
        }

        return m_audioManager->Initialize();
    }

    bool ControllerCore::InitializeRendererManager()
    {
        m_rendererManager = std::make_unique<RendererManager>(
            m_eventBus.get(),
            m_windowManager.get()
        );

        if (!m_rendererManager) {
            return false;
        }

        if (!m_rendererManager->Initialize()) {
            return false;
        }

        // Set initial renderer
        m_rendererManager->SetCurrentRenderer(
            m_rendererManager->GetCurrentStyle()
        );

        return true;
    }

    bool ControllerCore::InitializeUIManager()
    {
        auto* uiManager = m_windowManager->GetUIManager();
        if (!uiManager) {
            return false;
        }

        return uiManager->Initialize();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::MainLoop()
    {
        while (m_windowManager->IsRunning())
        {
            m_windowManager->ProcessMessages();

            if (ShouldProcessFrame())
            {
                const FrameState frameState = CollectFrameState();
                ProcessFrame(frameState);
                ++m_frameCounter;
            }
            else
            {
                ThrottleFrameRate();
            }
        }
    }

    void ControllerCore::ProcessFrame(const FrameState& frameState)
    {
        ProcessInput();
        Update(frameState);
        Render(frameState);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Frame State
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] FrameState ControllerCore::CollectFrameState() const
    {
        FrameState state;
        state.deltaTime = CalculateDeltaTime();
        state.frameNumber = m_frameCounter;
        state.isActive = m_windowManager->IsActive();
        state.isOverlayMode = m_windowManager->IsOverlayMode();
        state.mouse = CollectMouseState();

        return state;
    }

    [[nodiscard]] MouseState ControllerCore::CollectMouseState() const
    {
        MouseState state;

        const auto* messageHandler = m_windowManager->GetMessageHandler();
        if (!messageHandler) {
            return state;
        }

        const auto& mouseState = messageHandler->GetMouseState();
        state.position = mouseState.position;
        state.leftButtonDown = mouseState.leftButtonDown;
        state.rightButtonDown = mouseState.rightButtonDown;
        state.middleButtonDown = mouseState.middleButtonDown;
        state.wheelDelta = mouseState.wheelDelta;

        return state;
    }

    [[nodiscard]] float ControllerCore::CalculateDeltaTime() const
    {
        return kTargetFrameTime;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Input Processing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::ProcessInput()
    {
        if (!m_inputManager || !m_eventBus) {
            return;
        }

        UpdateInputManager();
        PublishInputActions();
    }

    void ControllerCore::UpdateInputManager()
    {
        m_inputManager->Update();
        m_actions = m_inputManager->GetActions();
    }

    void ControllerCore::PublishInputActions()
    {
        for (const auto& action : m_actions) {
            m_eventBus->Publish(action);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Phase
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::Update(const FrameState& frameState)
    {
        UpdateAudioSystem(frameState.deltaTime);
        UpdateUISystem(frameState);
    }

    void ControllerCore::UpdateAudioSystem(float deltaTime)
    {
        if (m_audioManager) {
            m_audioManager->Update(deltaTime);
        }
    }

    void ControllerCore::UpdateUISystem(const FrameState& frameState)
    {
        auto* uiManager = m_windowManager->GetUIManager();
        if (!uiManager) {
            return;
        }

        uiManager->Update(
            frameState.mouse.position,
            frameState.mouse.leftButtonDown,
            frameState.deltaTime
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Render Phase
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::Render(const FrameState& frameState)
    {
        if (!CanRender(frameState)) {
            return;
        }

        if (!PrepareFrame(frameState)) {
            return;
        }

        RenderContent();
        FinalizeFrame();
    }

    bool ControllerCore::PrepareFrame(const FrameState& frameState)
    {
        auto* engine = GetRenderEngine();
        if (!engine) {
            return false;
        }

        if (!engine->BeginDraw()) {
            LOG_ERROR("ControllerCore: Failed to begin drawing frame.");
            return false;
        }

        const Color clearColor = GetClearColor(frameState.isOverlayMode);
        engine->Clear(clearColor);

        return true;
    }

    void ControllerCore::RenderContent()
    {
        RenderSpectrum();
        RenderUI();
    }

    void ControllerCore::RenderSpectrum()
    {
        if (!ValidateAudioManager() || !ValidateRendererManager()) {
            return;
        }

        auto* engine = GetRenderEngine();
        if (!engine) {
            return;
        }

        auto* currentRenderer = m_rendererManager->GetCurrentRenderer();
        if (!currentRenderer) {
            return;
        }

        const SpectrumData spectrum = m_audioManager->GetSpectrum();
        currentRenderer->Render(engine->GetCanvas(), spectrum);
    }

    void ControllerCore::RenderUI()
    {
        if (m_windowManager->IsOverlayMode()) {
            return; // No UI in overlay mode
        }

        auto* engine = GetRenderEngine();
        if (!engine) {
            return;
        }

        auto* uiManager = m_windowManager->GetUIManager();
        if (!uiManager) {
            return;
        }

        uiManager->Draw(engine->GetCanvas());
    }

    void ControllerCore::FinalizeFrame()
    {
        auto* engine = GetRenderEngine();
        if (!engine) {
            return;
        }

        const HRESULT hr = engine->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET) {
            HandleDeviceLoss();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Render Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool ControllerCore::CanRender(const FrameState& frameState) const
    {
        if (frameState.isOverlayMode) {
            return ShouldRenderInOverlayMode();
        }

        return ShouldRenderInNormalMode(frameState);
    }

    [[nodiscard]] bool ControllerCore::IsWindowOccluded() const
    {
        if (m_windowManager && m_windowManager->IsOverlayMode()) {
            return false;
        }

        auto* engine = GetRenderEngine();
        if (!engine) {
            return true;
        }

        auto* renderTarget = engine->GetRenderTarget();
        if (!Helpers::RenderTarget::IsValid(renderTarget)) {
            return true;
        }

        return Helpers::RenderTarget::IsWindowOccluded(renderTarget);
    }

    [[nodiscard]] bool ControllerCore::ShouldRenderInOverlayMode() const
    {
        // Overlay windows are always rendered when the system is running
        // They are topmost, so occlusion check is not necessary
        return !IsWindowOccluded();
    }

    [[nodiscard]] bool ControllerCore::ShouldRenderInNormalMode(
        const FrameState& frameState
    ) const
    {
        return frameState.isActive;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Frame Timing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool ControllerCore::ShouldProcessFrame() const
    {
        return m_timer.GetElapsedSeconds() >= kTargetFrameTime;
    }

    void ControllerCore::ThrottleFrameRate() const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    void ControllerCore::ResetFrameTimer()
    {
        m_timer.Reset();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Graphics Recovery
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::HandleDeviceLoss()
    {
        if (!RecreateGraphicsContext()) {
            LOG_ERROR("ControllerCore: Failed to recreate graphics after device loss");
        }
    }

    bool ControllerCore::RecreateGraphicsContext()
    {
        const HWND hwnd = m_windowManager->GetCurrentHwnd();
        if (!hwnd) {
            return false;
        }

        return m_windowManager->RecreateGraphicsAndNotify(hwnd);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool ControllerCore::ValidateManagers() const noexcept
    {
        return ValidateWindowManager() &&
            ValidateAudioManager() &&
            ValidateRendererManager();
    }

    [[nodiscard]] bool ControllerCore::ValidateWindowManager() const noexcept
    {
        return m_windowManager != nullptr;
    }

    [[nodiscard]] bool ControllerCore::ValidateAudioManager() const noexcept
    {
        return m_audioManager != nullptr;
    }

    [[nodiscard]] bool ControllerCore::ValidateRendererManager() const noexcept
    {
        return m_rendererManager != nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Utility Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] RenderEngine* ControllerCore::GetRenderEngine() const noexcept
    {
        if (!m_windowManager) {
            return nullptr;
        }

        return m_windowManager->GetRenderEngine();
    }

    [[nodiscard]] Color ControllerCore::GetClearColor(bool isOverlayMode) const noexcept
    {
        if (isOverlayMode) {
            return Color::Transparent();
        }

        return Color::FromRGB(13, 13, 26);
    }

} // namespace Spectrum