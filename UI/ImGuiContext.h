#ifndef SPECTRUM_CPP_IMGUI_CONTEXT_H
#define SPECTRUM_CPP_IMGUI_CONTEXT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ImGuiContext — RAII wrapper for Dear ImGui (DX11 + Win32).
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND, UINT, WPARAM, LPARAM);

namespace Spectrum {

    class ImGuiContext final {
    public:
        ImGuiContext() = default;
        ~ImGuiContext() noexcept { Shutdown(); }

        ImGuiContext(const ImGuiContext&) = delete;
        ImGuiContext& operator=(const ImGuiContext&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* ctx) {
            if (m_init) Shutdown();
            if (!hwnd || !device || !ctx) return false;

            m_ctx = ctx;
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            auto& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename = nullptr;

            if (!ImGui_ImplWin32_Init(hwnd)) {
                ImGui::DestroyContext();
                return false;
            }
            if (!ImGui_ImplDX11_Init(device, ctx)) {
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();
                return false;
            }

            ApplyTheme();
            m_init = true;
            return true;
        }

        void Shutdown() {
            if (!m_init) return;
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            m_ctx.Reset();
            m_rtv.Reset();
            m_init = false;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Frame
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void BeginFrame() {
            if (!m_init) return;
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void EndFrame() {
            if (!m_init) return;
            ImGui::Render();
            if (auto* dd = ImGui::GetDrawData(); dd && m_ctx && m_rtv) {
                m_ctx->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);
                ImGui_ImplDX11_RenderDrawData(dd);
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Input / Accessors
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        bool ProcessMessage(HWND h, UINT m, WPARAM w, LPARAM l) {
            return m_init
                && ImGui_ImplWin32_WndProcHandler(h, m, w, l) != 0;
        }

        void SetRTV(ID3D11RenderTargetView* rtv) { m_rtv = rtv; }
        [[nodiscard]] bool IsReady() const noexcept { return m_init; }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Theme
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        static void ApplyTheme() {
            constexpr float R = 6.0f;

            auto& s = ImGui::GetStyle();
            s.WindowRounding = 0;   s.WindowBorderSize = 0;
            s.FrameRounding = R;   s.GrabRounding = R;
            s.ScrollbarRounding = R;   s.TabRounding = R;
            s.ChildRounding = R;   s.PopupRounding = R;

            s.WindowPadding = { 16, 16 };
            s.FramePadding = { 10,  6 };
            s.ItemSpacing = { 8,  8 };
            s.ItemInnerSpacing = { 6,  4 };
            s.IndentSpacing = 22;
            s.ScrollbarSize = 10;
            s.GrabMinSize = 10;
            s.WindowMinSize = { 100, 100 };

            const ImVec4 bg = { 0.055f, 0.055f, 0.09f, 1 };
            const ImVec4 sf = { 0.08f,  0.08f,  0.12f, 1 };
            const ImVec4 sfH = { 0.10f,  0.10f,  0.155f,1 };
            const ImVec4 ac = { 0.35f,  0.40f,  0.95f, 1 };
            const ImVec4 acD = { 0.20f,  0.24f,  0.60f, 1 };
            const ImVec4 txtP = { 0.92f,  0.92f,  0.96f, 1 };
            const ImVec4 txtS = { 0.50f,  0.50f,  0.60f, 1 };
            const ImVec4 brd = { 0.18f,  0.18f,  0.26f, 0.5f };

            auto* c = s.Colors;
            c[ImGuiCol_Text] = txtP;
            c[ImGuiCol_TextDisabled] = txtS;
            c[ImGuiCol_WindowBg] = bg;
            c[ImGuiCol_ChildBg] = { 0,0,0,0 };
            c[ImGuiCol_PopupBg] = { 0.07f, 0.07f, 0.11f, 0.96f };
            c[ImGuiCol_Border] = brd;
            c[ImGuiCol_BorderShadow] = { 0,0,0,0 };
            c[ImGuiCol_FrameBg] = sf;
            c[ImGuiCol_FrameBgHovered] = sfH;
            c[ImGuiCol_FrameBgActive] = { 0.14f, 0.14f, 0.22f, 1 };
            c[ImGuiCol_TitleBg] = bg;
            c[ImGuiCol_TitleBgActive] = bg;
            c[ImGuiCol_TitleBgCollapsed] = bg;
            c[ImGuiCol_ScrollbarBg] = { 0.05f, 0.05f, 0.08f, 0.6f };
            c[ImGuiCol_ScrollbarGrab] = { 0.22f, 0.22f, 0.32f, 1 };
            c[ImGuiCol_ScrollbarGrabHovered] = { 0.30f, 0.30f, 0.42f, 1 };
            c[ImGuiCol_ScrollbarGrabActive] = ac;
            c[ImGuiCol_CheckMark] = ac;
            c[ImGuiCol_SliderGrab] = acD;
            c[ImGuiCol_SliderGrabActive] = ac;
            c[ImGuiCol_Button] = sf;
            c[ImGuiCol_ButtonHovered] = sfH;
            c[ImGuiCol_ButtonActive] = acD;
            c[ImGuiCol_Header] = sf;
            c[ImGuiCol_HeaderHovered] = sfH;
            c[ImGuiCol_HeaderActive] = { 0.16f, 0.16f, 0.24f, 1 };
            c[ImGuiCol_Separator] = brd;
            c[ImGuiCol_SeparatorHovered] = { 0.28f, 0.28f, 0.40f, 0.8f };
            c[ImGuiCol_SeparatorActive] = ac;
            c[ImGuiCol_ResizeGrip] = { 0.2f, 0.2f, 0.3f, 0.25f };
            c[ImGuiCol_ResizeGripHovered] = acD;
            c[ImGuiCol_ResizeGripActive] = ac;
            c[ImGuiCol_TextSelectedBg] = { ac.x, ac.y, ac.z, 0.3f };
            c[ImGuiCol_NavHighlight] = ac;
            c[ImGuiCol_ModalWindowDimBg] = { 0, 0, 0, 0.6f };
        }

        bool m_init = false;
        wrl::ComPtr<ID3D11DeviceContext>    m_ctx;
        wrl::ComPtr<ID3D11RenderTargetView> m_rtv;
    };

} // namespace Spectrum

#endif