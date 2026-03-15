#include "ControllerCore.h"

#include "Audio/AudioManager.h"
#include "Common/EventBus.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "Platform/InputManager.h"
#include "Platform/MainWindow.h"
#include "Platform/MessageHandler.h"
#include "Platform/WindowManager.h"
#include "UI/UIManager.h"

#include <thread>

namespace Spectrum {

    namespace {
        constexpr Color kClearColor = Color::FromRGB(13, 13, 26);
        constexpr Color kUIBackground = Color::FromRGB(30, 30, 40);
        constexpr float kBtnSize = 30.0f;
        constexpr float kBtnPad = 10.0f;
        constexpr float kBtnFontSize = 24.0f;
        constexpr float kBtnAlphaIdle = 0.5f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ControllerCore::ControllerCore(HINSTANCE hInstance)
        : m_hInstance(hInstance) {
    }

    ControllerCore::~ControllerCore() noexcept {
        Shutdown();
    }

    bool ControllerCore::Initialize() {
        if (!InitializeSubsystems()) return false;
        m_timer.Reset();
        return true;
    }

    void ControllerCore::Run() {
        if (!m_windowMgr) return;
        m_timer.Reset();
        m_frameCounter = 0;
        MainLoop();
    }

    void ControllerCore::Shutdown() {
        if (m_audioMgr) m_audioMgr->Shutdown();
        m_rendererMgr.reset();
        m_audioMgr.reset();
        m_inputMgr.reset();
        m_windowMgr.reset();
        m_eventBus.reset();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event callbacks
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::OnResize(int w, int h) {
        if (m_rendererMgr) m_rendererMgr->OnResize(w, h);
    }

    void ControllerCore::OnUIResize(int w, int h) {
        if (auto* ui = m_windowMgr ? m_windowMgr->GetUIManager() : nullptr)
            ui->OnResize(w, h);
    }

    void ControllerCore::OnCloseRequest() {
        if (m_windowMgr)
            if (auto* mw = m_windowMgr->GetMainWindow())
                mw->SetRunning(false);
    }

    void ControllerCore::OnMainWindowClick(const Point& pos) {
        if (!m_windowMgr || !m_settingsBtnRect.Contains(pos)) return;

        m_windowMgr->IsUIWindowVisible()
            ? m_windowMgr->HideUIWindow()
            : m_windowMgr->ShowUIWindow();
    }

    void ControllerCore::SetPrimaryColor(const Color& color) {
        if (m_rendererMgr)
            if (auto* r = m_rendererMgr->GetCurrentRenderer())
                r->SetPrimaryColor(color);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool ControllerCore::InitializeSubsystems() {
        m_eventBus = std::make_unique<EventBus>();

        m_windowMgr = std::make_unique<Platform::WindowManager>(
            m_hInstance, this, m_eventBus.get());
        if (!m_windowMgr->Initialize()) return false;

        m_inputMgr = std::make_unique<Platform::InputManager>();

        m_audioMgr = std::make_unique<AudioManager>(m_eventBus.get());
        if (!m_audioMgr->Initialize()) return false;

        m_rendererMgr = std::make_unique<RendererManager>(
            m_eventBus.get(), m_windowMgr.get());
        if (!m_rendererMgr->Initialize()) return false;

        m_rendererMgr->SetCurrentRenderer(m_rendererMgr->GetCurrentStyle());
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::MainLoop() {
        MSG msg{};

        while (m_windowMgr->IsRunning()) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!m_windowMgr->IsRunning()) break;

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
        const auto fs = CollectFrameState();

        ProcessInput(fs.deltaTime);

        if (fs.isOverlay || fs.isActive)
            RenderVisualization(fs);

        if (m_windowMgr && m_windowMgr->IsUIWindowVisible())
            RenderUI();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Frame state
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    FrameState ControllerCore::CollectFrameState() const {
        FrameState fs;
        fs.deltaTime = kFrameTime;
        fs.frameNumber = m_frameCounter;
        fs.isActive = m_windowMgr->IsActive();
        fs.isOverlay = m_windowMgr->IsOverlayMode();

        if (auto* mh = m_windowMgr->GetMessageHandler())
            fs.mouse = mh->GetMouseState();

        return fs;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Input
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::ProcessInput(float dt) {
        if (!m_inputMgr || !m_eventBus) return;

        m_inputMgr->Update();

        for (const auto& action : m_inputMgr->FlushActions())
            m_eventBus->Publish(action);

        if (m_audioMgr)
            m_audioMgr->Update(dt);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControllerCore::RenderVisualization(const FrameState& fs) {
        auto* engine = m_windowMgr->GetVisualizationEngine();
        if (!engine || !engine->BeginDraw()) return;

        engine->Clear(fs.isOverlay ? Color::Transparent() : kClearColor);

        if (m_audioMgr && m_rendererMgr)
            if (auto* r = m_rendererMgr->GetCurrentRenderer())
                r->Render(engine->GetCanvas(), m_audioMgr->GetSpectrum());

        RenderSettingsButton(fs);

        if (engine->EndDraw() == D2DERR_RECREATE_TARGET)
            HandleDeviceLoss(engine);
    }

    void ControllerCore::RenderUI() {
        auto* ui = m_windowMgr->GetUIManager();
        auto* engine = m_windowMgr->GetUIEngine();
        if (!ui || !engine) return;

        engine->ClearD3D11(kUIBackground);
        ui->BeginFrame();
        ui->Render();
        ui->EndFrame();
        engine->Present();
    }

    void ControllerCore::RenderSettingsButton(const FrameState& fs) {
        auto* engine = m_windowMgr->GetVisualizationEngine();
        if (!engine) return;

        const float x = static_cast<float>(engine->GetWidth()) - kBtnSize - kBtnPad;
        m_settingsBtnRect = Rect(x, kBtnPad, kBtnSize, kBtnSize);

        const bool hovered = m_settingsBtnRect.Contains(fs.mouse.position);

        const TextStyle style = TextStyle::Default()
            .WithFont(L"Segoe UI Symbol")
            .WithSize(kBtnFontSize)
            .WithAlign(TextAlign::Center)
            .WithParagraphAlign(ParagraphAlign::Center)
            .WithColor(Color(1, 1, 1, hovered ? 1.0f : kBtnAlphaIdle));

        engine->GetCanvas().DrawText(L"\u2699", m_settingsBtnRect, style);
    }

    void ControllerCore::HandleDeviceLoss(RenderEngine* engine) {
        if (!m_windowMgr || !engine) return;

        const bool isUI = (engine == m_windowMgr->GetUIEngine());
        const bool ok = isUI
            ? m_windowMgr->HandleUIResize(engine->GetWidth(), engine->GetHeight(), true)
            : m_windowMgr->HandleVisualizationResize(engine->GetWidth(), engine->GetHeight(), true);

        if (!ok)
            LOG_ERROR("ControllerCore: Failed to recover from device loss ("
                << (isUI ? "UI" : "viz") << ")");
    }

} // namespace Spectrum