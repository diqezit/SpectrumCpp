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
        if (!m_ctx) return;
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
                ui::BindCombo("Renderer", m_renderer,
                    &RendererManager::GetCurrentRendererName,
                    &RendererManager::GetAvailableRendererNames,
                    &RendererManager::SetCurrentRendererByName);
            }

            if (ui::Section s("AUDIO"); s) {
                UI_SLIDER(m_audio, "Amplification", Amplification);
                UI_SLIDER(m_audio, "Smoothing", Smoothing);
                UI_SLIDER(m_audio, "Bar Count", BarCount);

                ui::BindCombo("FFT Window", m_audio,
                    &AudioManager::GetFFTWindowName,
                    &AudioManager::GetAvailableFFTWindows,
                    &AudioManager::SetFFTWindowByName);

                ui::BindCombo("Scale", m_audio,
                    &AudioManager::GetSpectrumScaleName,
                    &AudioManager::GetAvailableSpectrumScales,
                    &AudioManager::SetSpectrumScaleByName);

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

        const bool capturing = m_audio->IsCapturing();
        const bool animating = m_audio->IsAnimating();

        char buf[128];
        snprintf(buf, sizeof(buf), "%s   %d bars   %s   %s",
            capturing ? "Capturing" : animating ? "Animation" : "Idle",
            int(m_audio->GetBarCount()),
            m_audio->GetFFTWindowName().data(),
            m_audio->GetSpectrumScaleName().data());

        ui::StatusBar(
            ImGui::GetWindowPos().y + root.h - ui::kStatusH, root.w, buf,
            capturing ? ui::Pal().statusOn : animating ? ui::Pal().statusWarn : ui::Pal().statusOff);
    }

} // namespace Spectrum

#endif