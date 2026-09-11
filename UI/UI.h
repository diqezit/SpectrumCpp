#ifndef SPECTRUM_CPP_UI_H
#define SPECTRUM_CPP_UI_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Settings panel. Values come from Audio / Renderer.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "UI/ImGuiContext.h"
#include "UI/Widgets.h"
#include "Audio/Audio.h"
#include "Graphics/RendererManager.h"

#include <cstdio>
#include <functional>
#include <memory>
#include <string>

namespace Spectrum {

    class UIManager final {
    public:
        UIManager(std::function<void()> onHide, std::function<void()> onOverlay)
            : m_onHide(std::move(onHide))
            , m_onOverlay(std::move(onOverlay)) {
        }

        ~UIManager() noexcept { Shutdown(); }

        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;
        UIManager(UIManager&&) = delete;
        UIManager& operator=(UIManager&&) = delete;

        void Attach(AudioManager* audio, RendererManager* renderer) {
            m_audio = audio;
            m_renderer = renderer;
        }

        [[nodiscard]] bool Initialize(
            HWND hwnd, ID3D11Device* device,
            ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv);

        void Shutdown();

        void BeginFrame() { m_ctx->BeginFrame(); }
        void Render() { DrawPanel(); }
        void EndFrame() { m_ctx->EndFrame(); }

        void OnResize(ID3D11RenderTargetView* rtv) { m_ctx->SetRTV(rtv); }

        [[nodiscard]] bool HandleMessage(HWND h, UINT m, WPARAM w, LPARAM l) {
            return m_ctx->ProcessMessage(h, m, w, l);
        }

    private:
        void DrawPanel();
        void DrawStatus(float y, float width);

        std::function<void()>         m_onHide;
        std::function<void()>         m_onOverlay;
        AudioManager* m_audio = nullptr;
        RendererManager* m_renderer = nullptr;
        std::unique_ptr<ImGuiContext> m_ctx;
        Color                         m_color = Color::White();
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool UIManager::Initialize(
        HWND hwnd, ID3D11Device* device,
        ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv)
    {
        m_ctx = std::make_unique<ImGuiContext>();
        if (!m_ctx->Initialize(hwnd, device, ctx))
            return false;
        m_ctx->SetRTV(rtv);
        return true;
    }

    inline void UIManager::Shutdown() {
        if (!m_ctx)
            return;
        m_ctx->Shutdown();
        m_ctx.reset();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Panel
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline void UIManager::DrawPanel() {
        ui::Fullscreen root("##root");
        if (ui::TitleBar(root.w, "SPECTRUM"))
            m_onHide();

        {
            ui::Body body(root.h - ui::kTitleH - ui::kStatusH);

            if (ui::Section s("RENDERER"); s) {
                ui::NamedCombo("Renderer",
                    m_renderer->GetCurrentRendererName(),
                    m_renderer->GetAvailableRendererNames(),
                    [this](const std::string& n) { m_renderer->SetCurrentRendererByName(n); });
            }

            if (ui::Section s("AUDIO"); s) {
                float amp = m_audio->GetAmplification();
                if (ui::Slider("Amplification", &amp,
                    Analyzer::kAmpMin, Analyzer::kAmpMax))
                    m_audio->SetAmplification(amp);

                float smooth = m_audio->GetSmoothing();
                if (ui::Slider("Smoothing", &smooth,
                    Analyzer::kSmoothMin, Analyzer::kSmoothMax))
                    m_audio->SetSmoothing(smooth);

                size_t bars = m_audio->GetBarCount();
                if (ui::Slider("Bar Count", &bars,
                    AudioManager::kMinBarCount, size_t(Analyzer::MAX_BARS)))
                    m_audio->SetBarCount(bars);

                ui::NamedCombo("FFT Window",
                    m_audio->GetFFTWindowName(),
                    m_audio->GetAvailableFFTWindows(),
                    [this](const std::string& n) { m_audio->SetFFTWindowByName(n); });

                if (ui::NeonButton("Reset to Defaults", ui::Pal().accent))
                    m_audio->ResetToDefaults();
            }

            if (ui::Section s("COLOR"); s) {
                if (ui::ColorPicker(m_color))
                    m_renderer->GetCurrentRenderer()->SetPrimaryColor(m_color);
            }

            if (ui::Section s("DISPLAY"); s) {
                if (ui::NeonButton("Toggle Overlay", ui::Pal().statusOn))
                    m_onOverlay();
            }
        }

        DrawStatus(ImGui::GetWindowPos().y + root.h - ui::kStatusH, root.w);
    }

    inline void UIManager::DrawStatus(float y, float width) {
        const auto& pal = ui::Pal();

        const char* name = "Idle";
        ImVec4 color = pal.statusOff;
        if (m_audio->IsCapturing()) {
            name = "Capturing";
            color = pal.statusOn;
        }

        char buf[128];
        snprintf(buf, sizeof(buf), "%s   %d bars   %s",
            name,
            int(m_audio->GetBarCount()),
            m_audio->GetFFTWindowName().data());

        ui::StatusBar(y, width, buf, color);
    }

} // namespace Spectrum

#endif