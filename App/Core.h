#ifndef SPECTRUM_CPP_CORE_H
#define SPECTRUM_CPP_CORE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Owns subsystems, pumps the frame, routes input / resize / overlay.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Audio/Audio.h"
#include "Common/EventBus.h"
#include "Graphics/RendererManager.h"
#include "Graphics/API/D3D11Backend.h"
#include "Graphics/API/Draw.h"
#include "Graphics/API/GraphicsSurface.h"
#include "Platform/InputManager.h"
#include "Platform/MainWindow.h"
#include "Platform/WindowManager.h"
#include "UI/UI.h"

#include <thread>

namespace Spectrum {

    struct FrameState {
        Platform::MouseState mouse;
        float    deltaTime = 0.0f;
        uint64_t frameNumber = 0;
        bool     isActive = false;
        bool     isOverlay = false;
    };

    class Core final {
    public:
        explicit Core(HINSTANCE hInstance) : m_hInstance(hInstance) {}
        ~Core() noexcept { Shutdown(); }

        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] bool Initialize() {
            m_eventBus = std::make_unique<EventBus>();
            m_windowMgr = std::make_unique<Platform::WindowManager>(
                m_hInstance, this, m_eventBus.get(),
                [this](int w, int h) { OnResize(w, h); },
                [this](int, int) { OnUIResize(); },
                [this](bool overlay) { SetOverlayMode(overlay); });

            if (!m_windowMgr->Initialize()) return false;

            m_inputMgr = std::make_unique<Platform::InputManager>();

            m_audioMgr = std::make_unique<AudioManager>(m_eventBus.get());
            if (!m_audioMgr->Initialize()) return false;

            auto* surface = m_windowMgr->GetVisualizationSurface();
            m_rendererMgr = std::make_unique<RendererManager>(m_eventBus.get());
            if (!m_rendererMgr->Initialize(surface->GetWidth(), surface->GetHeight()))
                return false;

            m_windowMgr->GetUIManager()->Attach(m_audioMgr.get(), m_rendererMgr.get());
            m_windowMgr->ForceUIRender();
            m_timer.Reset();
            return true;
        }

        void Run() {
            m_timer.Reset();
            m_frameCounter = 0;
            MainLoop();
        }

        void Shutdown() {
            m_rendererMgr.reset();
            m_audioMgr.reset();
            m_inputMgr.reset();
            m_windowMgr.reset();
            m_eventBus.reset();
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Window / UI hooks
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void OnResize(int w, int h) {
            if (m_rendererMgr) m_rendererMgr->OnResize(w, h);
        }

        void OnUIResize() {
            m_windowMgr->GetUIManager()->OnResize(
                m_windowMgr->GetUIBackend()->GetD3D11RenderTargetView());
        }

        void OnCloseRequest() {
            m_windowMgr->GetMainWindow()->SetRunning(false);
        }

        void OnMainWindowClick(const Point& pos) {
            if (!m_settingsBtnRect.Contains(pos)) return;
            m_windowMgr->IsUIWindowVisible()
                ? m_windowMgr->HideUIWindow()
                : m_windowMgr->ShowUIWindow();
        }

        void SetPrimaryColor(const Color& color) {
            m_rendererMgr->GetCurrentRenderer()->SetPrimaryColor(color);
        }

        void SetOverlayMode(bool overlay) {
            m_rendererMgr->GetCurrentRenderer()->SetOverlayMode(overlay);
        }

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept { return m_rendererMgr.get(); }
        [[nodiscard]] AudioManager* GetAudioManager()    const noexcept { return m_audioMgr.get(); }
        [[nodiscard]] Platform::WindowManager* GetWindowManager()   const noexcept { return m_windowMgr.get(); }

    private:
        static constexpr float kFps = 60.0f;
        static constexpr float kFrameTime = 1.0f / kFps;
        static constexpr Color kClear = Color::FromRGB(13, 13, 26);
        static constexpr Color kUiBg = Color::FromRGB(30, 30, 40);
        static constexpr float kBtnSize = 30.0f;
        static constexpr float kBtnPad = 10.0f;
        static constexpr float kBtnFont = 24.0f;
        static constexpr float kBtnIdle = 0.5f;

        void MainLoop() {
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

        void ProcessFrame() {
            const auto fs = CollectFrameState();
            ProcessInput(fs.deltaTime);
            if (fs.isOverlay || fs.isActive) RenderVisualization(fs);
            if (m_windowMgr->IsUIWindowVisible()) RenderUI();
        }

        [[nodiscard]] FrameState CollectFrameState() const {
            return {
                m_windowMgr->GetMessageHandler()->GetMouseState(),
                kFrameTime,
                m_frameCounter,
                m_windowMgr->IsActive(),
                m_windowMgr->IsOverlayMode()
            };
        }

        void ProcessInput(float dt) {
            m_inputMgr->Update();
            for (const auto& action : m_inputMgr->FlushActions())
                m_eventBus->Publish(action);
            m_audioMgr->Update(dt);
        }

        void RenderVisualization(const FrameState& fs) {
            auto* surface = m_windowMgr->GetVisualizationSurface();
            if (!surface->BeginFrame()) return;

            surface->Clear(fs.isOverlay ? Color::Transparent() : kClear);
            m_rendererMgr->GetCurrentRenderer()->Render(
                surface->GetContext(), m_audioMgr->GetSpectrum());
            RenderSettingsButton(fs, surface);
            surface->EndFrame();
        }

        void RenderUI() {
            auto* ui = m_windowMgr->GetUIManager();
            auto* backend = m_windowMgr->GetUIBackend();

            backend->Clear(kUiBg);
            ui->BeginFrame();
            ui->Render();
            ui->EndFrame();
            if (!backend->Present()) HandleUIDeviceLoss();
        }

        void RenderSettingsButton(const FrameState& fs, GraphicsSurface* surface) {
            const float x = float(surface->GetWidth()) - kBtnSize - kBtnPad;
            m_settingsBtnRect = Rect(x, kBtnPad, kBtnSize, kBtnSize);

            const bool hovered = m_settingsBtnRect.Contains(fs.mouse.position);
            Draw::FillTextCentered(
                surface->GetContext(),
                "\xE2\x9A\x99",
                m_settingsBtnRect,
                Color(1.0f, 1.0f, 1.0f, hovered ? 1.0f : kBtnIdle),
                kBtnFont);
        }

        void HandleUIDeviceLoss() {
            auto* backend = m_windowMgr->GetUIBackend();
            static_cast<void>(m_windowMgr->HandleUIResize(
                backend->GetWidth(), backend->GetHeight(), true));
        }

        [[nodiscard]] bool ShouldProcessFrame() const {
            return m_timer.GetElapsedSeconds() >= kFrameTime;
        }

        HINSTANCE m_hInstance;

        std::unique_ptr<EventBus>                m_eventBus;
        std::unique_ptr<Platform::WindowManager> m_windowMgr;
        std::unique_ptr<AudioManager>            m_audioMgr;
        std::unique_ptr<RendererManager>         m_rendererMgr;
        std::unique_ptr<Platform::InputManager>  m_inputMgr;

        Helpers::Utils::Timer m_timer;
        uint64_t m_frameCounter = 0;
        Rect     m_settingsBtnRect;
    };

} // namespace Spectrum

#define SPECTRUM_CPP_CORE_READY
#include "Platform/Messages.h"

#endif