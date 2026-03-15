#include "UI/UIManager.h"
#include "UI/ImGuiContext.h"
#include "App/ControllerCore.h"
#include "Audio/AudioManager.h"
#include "Graphics/RendererManager.h"
#include "Platform/WindowManager.h"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace Spectrum {

    using T = UIManager::Theme;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ImU32  UIManager::U32(const ImVec4& c) { return ImGui::GetColorU32(c); }
    ImU32  UIManager::U32(float r, float g, float b, float a) { return ImGui::GetColorU32({ r,g,b,a }); }
    ImVec4 UIManager::WithAlpha(const ImVec4& c, float a) { return { c.x,c.y,c.z,a }; }

    void UIManager::DrawHoverGlow(const ImVec4& color) {
        if (!ImGui::IsItemHovered()) return;
        if (auto* dl = ImGui::GetWindowDrawList())
            dl->AddRect(
                ImGui::GetItemRectMin(),
                ImGui::GetItemRectMax(),
                U32(WithAlpha(color, T::alphaHover)),
                T::rounding, 0, 1.0f);
    }

    AudioManager* UIManager::Audio()    const { return m_ctrl ? m_ctrl->GetAudioManager() : nullptr; }
    RendererManager* UIManager::Renderer() const { return m_ctrl ? m_ctrl->GetRendererManager() : nullptr; }

    const char* UIManager::StyleName(RenderStyle s) {
        static constexpr const char* k[] = {
            "Bars", "Wave", "Circular Wave", "Cubes", "Fire",
            "LED Panel", "Gauge", "Kenwood Bars", "Particles",
            "Matrix LED", "Sphere", "Sunburst"
        };
        auto i = static_cast<size_t>(s);
        return i < static_cast<size_t>(RenderStyle::Count) ? k[i] : "Unknown";
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UIManager::UIManager(ControllerCore* ctrl, Platform::WindowManager* wm)
        : m_ctrl(ctrl), m_wm(wm)
    {
        if (!ctrl || !wm)
            throw std::invalid_argument("UIManager: null dependency");
    }

    UIManager::~UIManager() noexcept { Shutdown(); }

    bool UIManager::Initialize() {
        auto* engine = m_wm->GetUIEngine();
        HWND hwnd = m_wm->GetUIHwnd();
        if (!engine || !engine->IsD3D11Mode() || !hwnd) return false;

        m_ctx = std::make_unique<ImGuiContext>();
        if (!m_ctx->Initialize(hwnd,
            engine->GetD3D11Device(),
            engine->GetD3D11DeviceContext()))
            return false;

        if (auto* rtv = engine->GetD3D11RenderTargetView())
            m_ctx->SetRTV(rtv);

        return true;
    }

    void UIManager::Shutdown() {
        if (m_ctx) { m_ctx->Shutdown(); m_ctx.reset(); }
    }

    void UIManager::BeginFrame() {
        if (m_ctx && m_ctx->IsReady()) m_ctx->BeginFrame();
    }

    void UIManager::Render() {
        if (m_ctx && m_ctx->IsReady()) DrawPanel();
    }

    void UIManager::EndFrame() {
        if (m_ctx && m_ctx->IsReady()) m_ctx->EndFrame();
    }

    void UIManager::OnResize(int, int) {
        if (!m_ctx || !m_ctx->IsReady()) return;
        if (auto* e = m_wm->GetUIEngine())
            if (auto* rtv = e->GetD3D11RenderTargetView())
                m_ctx->SetRTV(rtv);
    }

    bool UIManager::HandleMessage(HWND h, UINT m, WPARAM w, LPARAM l) {
        return m_ctx && m_ctx->IsReady()
            && m_ctx->ProcessMessage(h, m, w, l);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main panel
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawPanel() {
        const auto& io = ImGui::GetIO();
        const float winW = io.DisplaySize.x;
        const float winH = io.DisplaySize.y;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ winW, winH });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::PushStyleColor(ImGuiCol_WindowBg, T::bg);

        constexpr auto kFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (!ImGui::Begin("##root", nullptr, kFlags)) {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::End();
            return;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        const ImVec2 winPos = ImGui::GetWindowPos();

        DrawTitleBar(winW);
        DrawAccentLine(winW);

        // Scrollable content
        const float contentH = T::scrollH(winH);
        if (contentH > 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                { T::contentPad, T::contentPad * 0.5f });
            ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,
                { 0.05f, 0.05f, 0.08f, 0.4f });
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,
                WithAlpha(T::accent, T::alphaDim));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,
                WithAlpha(T::accent, T::alphaSelect));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,
                T::accent);

            ImGui::BeginChild("##scroll", { 0, contentH }, false,
                ImGuiWindowFlags_AlwaysUseWindowPadding);

            ImGui::PopStyleColor(5);
            ImGui::PopStyleVar();

            ImGui::Spacing();
            DrawRendererSection();
            DrawAudioSection();
            DrawColorSection();
            DrawDisplaySection();
            ImGui::Spacing();

            ImGui::EndChild();
        }

        // Status bar — absolute bottom
        const float statusY = winPos.y + winH - T::statusH;
        DrawStatusBar(winW, statusY);

        ImGui::End();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Title bar
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawTitleBar(float winW) {
        auto* dl = ImGui::GetWindowDrawList();
        if (!dl) return;

        const ImVec2 p = ImGui::GetCursorScreenPos();

        dl->AddRectFilledMultiColor(
            p, { p.x + winW, p.y + T::titleH },
            U32(0.09f, 0.09f, 0.16f),
            U32(0.07f, 0.07f, 0.12f),
            U32(0.07f, 0.07f, 0.12f),
            U32(0.09f, 0.09f, 0.16f));

        dl->AddRectFilledMultiColor(
            { p.x, p.y + T::titleH - T::titleShadowH },
            { p.x + winW, p.y + T::titleH },
            U32(0, 0, 0, 0), U32(0, 0, 0, 0),
            U32(0, 0, 0, T::alphaShadowBg),
            U32(0, 0, 0, T::alphaShadowBg));

        const float iy = p.y + T::titleH * 0.5f;
        dl->AddCircleFilled({ p.x + T::titleIconX, iy },
            T::titleGlowR, U32(WithAlpha(T::accent, T::alphaGlow)));
        dl->AddCircleFilled({ p.x + T::titleIconX, iy },
            T::titleIconR, U32(WithAlpha(T::accent, T::alphaGlowHi)));

        ImGui::SetCursorPos({ T::titleTextX,
            (T::titleH - ImGui::GetFontSize()) * 0.5f });
        ImGui::PushStyleColor(ImGuiCol_Text, T::textAccent);
        ImGui::TextUnformatted("SPECTRUM");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos({
            winW - T::titleBtnSz - T::titleBtnMarg,
            (T::titleH - T::titleBtnSz) * 0.5f });

        ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, T::closeHov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, T::closeAct);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, T::rounding);

        if (ImGui::Button("##close", { T::titleBtnSz, T::titleBtnSz })
            && m_wm)
            m_wm->HideUIWindow();

        const ImVec2 bm = ImGui::GetItemRectMin();
        const ImVec2 bx = ImGui::GetItemRectMax();
        const float cx = (bm.x + bx.x) * 0.5f;
        const float cy = (bm.y + bx.y) * 0.5f;
        const ImU32 xc = ImGui::IsItemHovered()
            ? U32(T::textPri) : U32(T::textSec);

        dl->AddLine({ cx - T::titleXHalf, cy - T::titleXHalf },
            { cx + T::titleXHalf, cy + T::titleXHalf }, xc, T::titleXStroke);
        dl->AddLine({ cx + T::titleXHalf, cy - T::titleXHalf },
            { cx - T::titleXHalf, cy + T::titleXHalf }, xc, T::titleXStroke);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::SetCursorPosY(T::titleH);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Accent line
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawAccentLine(float winW) {
        auto* dl = ImGui::GetWindowDrawList();
        if (!dl) return;

        const ImVec2 p = ImGui::GetCursorScreenPos();

        dl->AddRectFilledMultiColor(
            p, { p.x + winW, p.y + T::accentLineH },
            U32(T::accent), U32(WithAlpha(T::accent, T::alphaGlowHi)),
            U32(WithAlpha(T::accent, T::alphaGlowHi)), U32(T::accent));

        dl->AddRectFilledMultiColor(
            { p.x, p.y + T::accentLineH },
            { p.x + winW, p.y + T::accentLineH + T::accentGlowH },
            U32(WithAlpha(T::accent, T::alphaGlowSoft)), U32(0, 0, 0, 0),
            U32(0, 0, 0, 0), U32(WithAlpha(T::accent, T::alphaGlowSoft)));

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + T::accentLineH);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Section
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool UIManager::BeginSection(const char* label) {
        ImGui::Spacing();
        m_sectionY = ImGui::GetCursorScreenPos().y;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
            { T::framePad, T::framePad });
        ImGui::PushStyleColor(ImGuiCol_Header, { 0,0,0,0 });
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
            WithAlpha(T::accent, T::alphaGlowSoft));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,
            WithAlpha(T::accent, 0.10f));

        char buf[128];
        snprintf(buf, sizeof(buf), "     %s", label);
        bool open = ImGui::CollapsingHeader(buf,
            ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (auto* dl = ImGui::GetWindowDrawList()) {
            const float midY = (ImGui::GetItemRectMin().y
                + ImGui::GetItemRectMax().y) * 0.5f;
            const float dotX = ImGui::GetItemRectMin().x
                + T::sectionDotOfs;

            dl->AddCircleFilled({ dotX, midY },
                T::sectionDotR, U32(T::accent));
            dl->AddCircle({ dotX, midY },
                T::sectionGlowR, U32(WithAlpha(T::accent, T::alphaSelect)));
        }

        if (open) {
            ImGui::Indent(T::sectionPad);
            ImGui::PushItemWidth(-1);   // full width for all items
            ImGui::Spacing();
        }

        return open;
    }

    void UIManager::EndSection() {
        ImGui::PopItemWidth();
        ImGui::Spacing();
        ImGui::Unindent(T::sectionPad);

        if (auto* dl = ImGui::GetWindowDrawList()) {
            const float endY = ImGui::GetCursorScreenPos().y;
            const float x = ImGui::GetWindowPos().x + T::borderX;

            dl->AddRectFilled(
                { x, m_sectionY + T::borderPad },
                { x + T::borderW, endY - T::borderPad },
                U32(WithAlpha(T::accent, T::alphaDim)), 1.0f);
        }

        ImGui::Spacing();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Custom slider
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawSliderTrack(ImVec2 bbMin, ImVec2 bbMax, float t) {
        auto* dl = ImGui::GetWindowDrawList();
        if (!dl) return;

        const float y = (bbMin.y + bbMax.y - T::sliderH) * 0.5f;
        const bool hov = ImGui::IsItemHovered() || ImGui::IsItemActive();

        dl->AddRectFilled({ bbMin.x, y }, { bbMax.x, y + T::sliderH },
            U32(T::trackBg), T::sliderH * 0.5f);

        const float fillX = bbMin.x + (bbMax.x - bbMin.x) * t;
        if (fillX > bbMin.x + 1)
            dl->AddRectFilledMultiColor(
                { bbMin.x, y }, { fillX, y + T::sliderH },
                U32(T::accentDim), U32(T::trackFill),
                U32(T::trackFill), U32(T::accentDim));

        const float gy = (bbMin.y + bbMax.y) * 0.5f;
        dl->AddCircleFilled({ fillX, gy + T::grabShadow },
            T::grabR + T::grabShadow, U32(0, 0, 0, T::alphaShadow));
        dl->AddCircleFilled({ fillX, gy }, T::grabR,
            U32(hov ? T::grabActive : T::grabColor));
        dl->AddCircleFilled({ fillX, gy }, T::grabInnerR,
            U32(hov ? T::accent : T::accentDim));
    }

    bool UIManager::FancySliderFloat(
        const char* label, float* v,
        float mn, float mx, const char* fmt)
    {
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), fmt,
            static_cast<double>(*v));

        // Label left, value right-aligned
        ImGui::PushStyleColor(ImGuiCol_Text, T::textPri);
        ImGui::TextUnformatted(label);
        ImGui::SameLine(
            ImGui::GetWindowContentRegionMax().x
            - ImGui::CalcTextSize(valBuf).x);
        ImGui::PushStyleColor(ImGuiCol_Text, T::textAccent);
        ImGui::TextUnformatted(valBuf);
        ImGui::PopStyleColor(2);

        // Invisible slider for interaction
        const ImVec4 invis = { 0,0,0,0 };
        ImGui::PushStyleColor(ImGuiCol_FrameBg, invis);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, invis);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, invis);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, invis);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, invis);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
            { 0, T::framePad });
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 1.0f);

        char id[64];
        snprintf(id, sizeof(id), "##sl_%s", label);
        bool changed = ImGui::SliderFloat(id, v, mn, mx, "");

        const float t = (mx > mn) ? (*v - mn) / (mx - mn) : 0.0f;
        DrawSliderTrack(ImGui::GetItemRectMin(),
            ImGui::GetItemRectMax(), t);

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
        ImGui::Spacing();
        return changed;
    }

    bool UIManager::FancySliderInt(
        const char* label, int* v, int mn, int mx)
    {
        float fv = static_cast<float>(*v);
        bool changed = FancySliderFloat(label, &fv,
            static_cast<float>(mn), static_cast<float>(mx), "%.0f");
        if (changed) *v = static_cast<int>(fv);
        return changed;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Buttons
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool UIManager::AccentButton(const char* label) {
        ImGui::PushStyleColor(ImGuiCol_Button,
            WithAlpha(T::accent, 0.10f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            WithAlpha(T::accent, 0.22f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            WithAlpha(T::accent, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_Text, T::textAccent);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, T::rounding);

        bool clicked = ImGui::Button(label, { -1, T::btnH });
        DrawHoverGlow(T::accent);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        return clicked;
    }

    bool UIManager::GlowButton(const char* label, const ImVec4& color) {
        ImGui::PushStyleColor(ImGuiCol_Button,
            WithAlpha(color, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            WithAlpha(color, T::alphaSelect));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            WithAlpha(color, 0.40f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(
            std::min(color.x * 1.3f, 1.0f),
            std::min(color.y * 1.3f, 1.0f),
            std::min(color.z, 1.0f), 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, T::rounding);

        bool clicked = ImGui::Button(label, { -1, T::btnH });
        DrawHoverGlow(color);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        return clicked;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Combo — shared styling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::PushComboStyle() {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, T::rounding);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, T::surface);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, T::surfaceHov);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, T::popup);
    }

    void UIManager::PopComboStyle() {
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }

    void UIManager::DrawComboItems(
        const std::vector<std::string>& options,
        const std::string& current,
        const std::function<void(const std::string&)>& cb)
    {
        for (const auto& opt : options) {
            const bool sel = (current == opt);
            ImGui::PushStyleColor(ImGuiCol_Header,
                WithAlpha(T::accent, T::alphaDim));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                WithAlpha(T::accent, T::alphaSelect));
            if (ImGui::Selectable(opt.c_str(), sel)) cb(opt);
            if (sel) ImGui::SetItemDefaultFocus();
            ImGui::PopStyleColor(2);
        }
    }

    void UIManager::LabeledCombo(
        const char* label, std::string_view current,
        const std::vector<std::string>& options,
        const std::function<void(const std::string&)>& cb)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, T::textSec);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        PushComboStyle();
        std::string cur(current);
        char id[64];
        snprintf(id, sizeof(id), "##cb_%s", label);
        if (ImGui::BeginCombo(id, cur.c_str())) {
            DrawComboItems(options, cur, cb);
            ImGui::EndCombo();
        }
        PopComboStyle();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Renderer
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawRendererSection() {
        if (!BeginSection("RENDERER")) return;

        if (auto* rm = Renderer()) {
            PushComboStyle();

            const char* cur = StyleName(rm->GetCurrentStyle());
            if (ImGui::BeginCombo("##rnd", cur)) {
                for (int i = 0; i < static_cast<int>(RenderStyle::Count); ++i) {
                    const auto s = static_cast<RenderStyle>(i);
                    const bool sel = (rm->GetCurrentStyle() == s);
                    ImGui::PushStyleColor(ImGuiCol_Header,
                        WithAlpha(T::accent, T::alphaDim));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                        WithAlpha(T::accent, T::alphaSelect));
                    if (ImGui::Selectable(StyleName(s), sel))
                        rm->SetCurrentRenderer(s);
                    if (sel) ImGui::SetItemDefaultFocus();
                    ImGui::PopStyleColor(2);
                }
                ImGui::EndCombo();
            }

            PopComboStyle();
        }
        EndSection();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Audio
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawAudioSection() {
        if (!BeginSection("AUDIO")) return;

        auto* am = Audio();
        if (!am) { EndSection(); return; }

        float amp = am->GetAmplification();
        if (FancySliderFloat("Amplification", &amp, 0.1f, 5.0f))
            am->SetAmplification(amp);

        float sm = am->GetSmoothing();
        if (FancySliderFloat("Smoothing", &sm, 0.0f, 0.95f))
            am->SetSmoothing(sm);

        int bars = static_cast<int>(am->GetBarCount());
        if (FancySliderInt("Bar Count", &bars, 16, 128))
            am->SetBarCount(static_cast<size_t>(bars));

        ImGui::Spacing();

        LabeledCombo("FFT Window",
            am->GetFFTWindowName(),
            am->GetAvailableFFTWindows(),
            [am](const std::string& n) { am->SetFFTWindowByName(n); });

        ImGui::Spacing();

        LabeledCombo("Scale",
            am->GetSpectrumScaleName(),
            am->GetAvailableSpectrumScales(),
            [am](const std::string& n) { am->SetSpectrumScaleByName(n); });

        ImGui::Spacing();

        if (AccentButton("Reset to Defaults")) {
            am->SetAmplification(DEFAULT_AMPLIFICATION);
            am->SetSmoothing(DEFAULT_SMOOTHING);
            am->SetBarCount(DEFAULT_BAR_COUNT);
            am->SetFFTWindowByName("Hann");
            am->SetSpectrumScaleByName("Logarithmic");
        }

        EndSection();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawColorSection() {
        if (!BeginSection("COLOR")) return;

        float rgb[3] = { m_color.r, m_color.g, m_color.b };

        const float sz = ImGui::GetFrameHeight();
        ImGui::ColorButton("##preview",
            { rgb[0], rgb[1], rgb[2], 1.0f },
            ImGuiColorEditFlags_NoTooltip
            | ImGuiColorEditFlags_NoDragDrop,
            { sz, sz });

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, T::textSec);
        ImGui::TextUnformatted("Current color");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, T::rounding);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, T::surface);

        if (ImGui::ColorPicker3("##color", rgb,
            ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_NoAlpha
            | ImGuiColorEditFlags_NoInputs))
        {
            m_color = Color(rgb[0], rgb[1], rgb[2], 1.0f);
            if (m_ctrl) m_ctrl->SetPrimaryColor(m_color);
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        EndSection();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Display
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawDisplaySection() {
        if (!BeginSection("DISPLAY")) return;
        if (GlowButton("Toggle Overlay", { 0.3f, 0.8f, 0.5f, 1 })
            && m_wm)
            m_wm->ToggleOverlay();
        EndSection();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Status bar — clipped, absolute bottom
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawStatusBar(float winW, float barY) {
        auto* dl = ImGui::GetWindowDrawList();
        if (!dl) return;

        const float winX = ImGui::GetWindowPos().x;
        const ImVec2 barMin = { winX, barY };
        const ImVec2 barMax = { winX + winW, barY + T::statusH };

        // Background + border
        dl->AddRectFilled(barMin, barMax, U32(T::statusBg));
        dl->AddLine(barMin, { barMax.x, barMin.y }, U32(T::cardBorder));

        auto* am = Audio();
        if (!am) return;

        // Clip text to status bar bounds
        dl->PushClipRect(barMin, barMax, true);

        const bool active = am->IsCapturing() || am->IsAnimating();
        const float dx = barMin.x + T::statusDotX;
        const float dy = barMin.y + T::statusH * 0.5f;

        if (active) {
            dl->AddCircleFilled({ dx, dy }, T::statusGlowR1,
                U32(WithAlpha(T::statusOn, T::alphaGlow)));
            dl->AddCircleFilled({ dx, dy }, T::statusGlowR2,
                U32(WithAlpha(T::statusOn, T::alphaGlowMed)));
        }
        dl->AddCircleFilled({ dx, dy }, T::statusDotR,
            U32(active ? T::statusOn : T::statusOff));

        const char* mode = am->IsCapturing() ? "Capturing"
            : am->IsAnimating() ? "Animation"
            : "Idle";

        char buf[128];
        snprintf(buf, sizeof(buf),
            "%s  |  %d bars  |  %s  |  %s",
            mode, static_cast<int>(am->GetBarCount()),
            am->GetFFTWindowName().data(),
            am->GetSpectrumScaleName().data());

        const float textY = barMin.y
            + (T::statusH - ImGui::GetFontSize()) * 0.5f;

        dl->AddText({ dx + T::statusTextOfs, textY },
            U32(T::textDim), buf);

        dl->PopClipRect();
    }

} // namespace Spectrum