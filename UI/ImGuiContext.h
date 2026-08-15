#ifndef SPECTRUM_CPP_IMGUI_CONTEXT_H
#define SPECTRUM_CPP_IMGUI_CONTEXT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ImGuiContext — Dear ImGui (DX11 + Win32)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND, UINT, WPARAM, LPARAM);

namespace Spectrum {

    struct Palette {
        ImVec4 background{ 0.055f, 0.055f, 0.090f, 1.00f };
        ImVec4 surface{ 0.080f, 0.080f, 0.120f, 1.00f };
        ImVec4 surfaceHover{ 0.100f, 0.100f, 0.155f, 1.00f };
        ImVec4 surfaceActive{ 0.140f, 0.140f, 0.220f, 1.00f };
        ImVec4 accent{ 0.350f, 0.400f, 0.950f, 1.00f };
        ImVec4 accentDim{ 0.200f, 0.240f, 0.600f, 1.00f };
        ImVec4 textPrimary{ 0.920f, 0.920f, 0.960f, 1.00f };
        ImVec4 textSecondary{ 0.500f, 0.500f, 0.600f, 1.00f };
        ImVec4 border{ 0.180f, 0.180f, 0.260f, 0.50f };
        ImVec4 statusOn{ 0.350f, 0.850f, 0.450f, 1.00f };
        ImVec4 statusWarn{ 0.950f, 0.750f, 0.300f, 1.00f };
        ImVec4 statusOff{ 0.450f, 0.450f, 0.550f, 1.00f };
        ImVec4 closeHover{ 0.800f, 0.250f, 0.250f, 1.00f };
        ImVec4 closeActive{ 0.600f, 0.150f, 0.150f, 1.00f };
    };

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
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void EndFrame() {
            ImGui::Render();
            m_ctx->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Input / Accessors
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        bool ProcessMessage(HWND h, UINT m, WPARAM w, LPARAM l) {
            return ImGui_ImplWin32_WndProcHandler(h, m, w, l) != 0;
        }

        void SetRTV(ID3D11RenderTargetView* rtv) { m_rtv = rtv; }

        [[nodiscard]] static const Palette& Theme() {
            static const Palette palette{};
            return palette;
        }

    private:
        static void ApplyTheme() {
            constexpr float kRounding = 6.0f;
            const Palette& p = Theme();

            auto& s = ImGui::GetStyle();
            s.WindowRounding = 0.0f;
            s.WindowBorderSize = 0.0f;
            s.FrameRounding = kRounding;
            s.GrabRounding = kRounding;
            s.ScrollbarRounding = kRounding;
            s.TabRounding = kRounding;
            s.ChildRounding = kRounding;
            s.PopupRounding = kRounding;

            s.WindowPadding = { 16, 16 };
            s.FramePadding = { 10,  6 };
            s.ItemSpacing = { 8,  8 };
            s.ItemInnerSpacing = { 6,  4 };
            s.IndentSpacing = 22.0f;
            s.ScrollbarSize = 10.0f;
            s.GrabMinSize = 10.0f;
            s.WindowMinSize = { 100, 100 };

            auto* c = s.Colors;
            c[ImGuiCol_Text] = p.textPrimary;
            c[ImGuiCol_TextDisabled] = p.textSecondary;
            c[ImGuiCol_WindowBg] = p.background;
            c[ImGuiCol_ChildBg] = { 0, 0, 0, 0 };
            c[ImGuiCol_PopupBg] = { 0.07f, 0.07f, 0.11f, 0.96f };
            c[ImGuiCol_Border] = p.border;
            c[ImGuiCol_BorderShadow] = { 0, 0, 0, 0 };
            c[ImGuiCol_FrameBg] = p.surface;
            c[ImGuiCol_FrameBgHovered] = p.surfaceHover;
            c[ImGuiCol_FrameBgActive] = p.surfaceActive;
            c[ImGuiCol_TitleBg] = p.background;
            c[ImGuiCol_TitleBgActive] = p.background;
            c[ImGuiCol_TitleBgCollapsed] = p.background;
            c[ImGuiCol_ScrollbarBg] = { 0.05f, 0.05f, 0.08f, 0.60f };
            c[ImGuiCol_ScrollbarGrab] = { 0.22f, 0.22f, 0.32f, 1.00f };
            c[ImGuiCol_ScrollbarGrabHovered] = { 0.30f, 0.30f, 0.42f, 1.00f };
            c[ImGuiCol_ScrollbarGrabActive] = p.accent;
            c[ImGuiCol_CheckMark] = p.accent;
            c[ImGuiCol_SliderGrab] = p.accentDim;
            c[ImGuiCol_SliderGrabActive] = p.accent;
            c[ImGuiCol_Button] = p.surface;
            c[ImGuiCol_ButtonHovered] = p.surfaceHover;
            c[ImGuiCol_ButtonActive] = p.accentDim;
            c[ImGuiCol_Header] = p.surface;
            c[ImGuiCol_HeaderHovered] = p.surfaceHover;
            c[ImGuiCol_HeaderActive] = p.surfaceActive;
            c[ImGuiCol_Separator] = p.border;
            c[ImGuiCol_SeparatorHovered] = { 0.28f, 0.28f, 0.40f, 0.80f };
            c[ImGuiCol_SeparatorActive] = p.accent;
            c[ImGuiCol_ResizeGrip] = { 0.20f, 0.20f, 0.30f, 0.25f };
            c[ImGuiCol_ResizeGripHovered] = p.accentDim;
            c[ImGuiCol_ResizeGripActive] = p.accent;
            c[ImGuiCol_TextSelectedBg] = { p.accent.x, p.accent.y, p.accent.z, 0.30f };
            c[ImGuiCol_NavHighlight] = p.accent;
            c[ImGuiCol_ModalWindowDimBg] = { 0, 0, 0, 0.60f };
        }

        bool m_init = false;
        wrl::ComPtr<ID3D11DeviceContext>    m_ctx;
        wrl::ComPtr<ID3D11RenderTargetView> m_rtv;
    };

} // namespace Spectrum

#endif