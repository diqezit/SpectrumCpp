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
#include "Graphics/API/GraphicsSurface.h"
#include "Graphics/Base/RenderUtils.h"
#include "Platform/InputManager.h"
#include "Platform/Window.h"
#include "Platform/WindowManager.h"
#include "UI/UI.h"

#include <thread>

namespace Spectrum {

    struct FrameState {
        Platform::MouseState mouse;
        bool isOverlay = false;
    };

    class Core final {
    public:
        explicit Core(HINSTANCE hInstance) : m_hInstance(hInstance) {}
        ~Core() noexcept { Shutdown(); }

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

            if (!m_windowMgr->Initialize())
                return false;

            m_inputMgr = std::make_unique<Platform::InputManager>();
            m_audioMgr = std::make_unique<AudioManager>(m_eventBus.get());

            auto* surface = m_windowMgr->GetVisualizationSurface();
            m_rendererMgr = std::make_unique<RendererManager>(m_eventBus.get());
            if (!m_rendererMgr->Initialize(surface->GetWidth(), surface->GetHeight()))
                return false;

            m_windowMgr->GetUIManager()->Attach(m_audioMgr.get(), m_rendererMgr.get());
            m_windowMgr->ForceUIRender();
            return true;
        }

        void Run() {
            m_timer.Reset();
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
            if (m_rendererMgr)
                m_rendererMgr->OnResize(w, h);
        }

        void OnUIResize() {
            m_windowMgr->GetUIManager()->OnResize(
                m_windowMgr->GetUIBackend()->GetD3D11RenderTargetView());
        }

        void OnCloseRequest() {
            m_windowMgr->GetMainWindow()->SetRunning(false);
        }

        void OnMainWindowClick(const Point& pos) {
            if (!m_settingsBtnRect.Contains(pos))
                return;
            if (m_windowMgr->IsUIWindowVisible())
                m_windowMgr->HideUIWindow();
            else
                m_windowMgr->ShowUIWindow();
        }

        void SetPrimaryColor(const Color& color) {
            m_rendererMgr->SetPrimaryColor(color);
        }

        void SetOverlayMode(bool overlay) {
            m_rendererMgr->GetCurrentRenderer()->SetOverlayMode(overlay);
        }

        [[nodiscard]] RendererManager* GetRendererManager() const noexcept { return m_rendererMgr.get(); }
        [[nodiscard]] AudioManager* GetAudioManager() const noexcept { return m_audioMgr.get(); }
        [[nodiscard]] Platform::WindowManager* GetWindowManager() const noexcept { return m_windowMgr.get(); }

    private:
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
                if (!m_windowMgr->IsRunning())
                    break;

                if (m_timer.GetElapsedSeconds() >= FRAME_TIME) {
                    ProcessFrame();
                    m_timer.Reset();
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }

        void ProcessFrame() {
            const FrameState fs{
                m_windowMgr->GetMessageHandler()->GetMouseState(),
                m_windowMgr->IsOverlayMode()
            };

            if (m_windowMgr->AcceptsHotkeys()) {
                m_inputMgr->Update();
                for (const auto& action : m_inputMgr->FlushActions())
                    m_eventBus->Publish(action);
            }
            m_audioMgr->Update(FRAME_TIME);

            if (fs.isOverlay || m_windowMgr->IsActive())
                RenderVisualization(fs);

            if (m_windowMgr->IsUIWindowVisible())
                RenderUI();
        }

        void RenderVisualization(const FrameState& fs) {
            auto* surface = m_windowMgr->GetVisualizationSurface();
            if (!surface->BeginFrame())
                return;

            if (fs.isOverlay)
                surface->Clear(Color::Transparent());
            else
                surface->Clear(kClear);

            m_rendererMgr->GetCurrentRenderer()->Render(
                surface->GetContext(), m_audioMgr->GetSpectrum());

            if (!fs.isOverlay)
                RenderSettingsButton(fs, surface);

            static_cast<void>(surface->EndFrame());
        }

        void RenderUI() {
            auto* ui = m_windowMgr->GetUIManager();
            auto* backend = m_windowMgr->GetUIBackend();

            backend->Clear(kUiBg);
            ui->BeginFrame();
            ui->Render();
            ui->EndFrame();
            if (!backend->Present())
                static_cast<void>(m_windowMgr->HandleUIResize(backend->GetWidth(), backend->GetHeight(), true));
        }

        void RenderSettingsButton(const FrameState& fs, GraphicsSurface* surface) {
            const float x = float(surface->GetWidth()) - kBtnSize - kBtnPad;
            m_settingsBtnRect = Rect(x, kBtnPad, kBtnSize, kBtnSize);

            float alpha = kBtnIdle;
            if (m_settingsBtnRect.Contains(fs.mouse.position))
                alpha = 1.0f;

            Draw::FillTextCentered(
                surface->GetContext(),
                "\xE2\x9A\x99",
                m_settingsBtnRect,
                Color(1.0f, 1.0f, 1.0f, alpha),
                kBtnFont);
        }

        HINSTANCE m_hInstance;

        std::unique_ptr<EventBus>                m_eventBus;
        std::unique_ptr<Platform::WindowManager> m_windowMgr;
        std::unique_ptr<AudioManager>            m_audioMgr;
        std::unique_ptr<RendererManager>         m_rendererMgr;
        std::unique_ptr<Platform::InputManager>  m_inputMgr;

        Helpers::Utils::Timer m_timer;
        Rect m_settingsBtnRect;
    };

} // namespace Spectrum

#define SPECTRUM_CPP_CORE_READY
#include "Platform/Messages.h"

#endif