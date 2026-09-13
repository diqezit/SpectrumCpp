#ifndef SPECTRUM_CPP_IMGUI_CONTEXT_H
#define SPECTRUM_CPP_IMGUI_CONTEXT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ImGuiContext — Dear ImGui (DX11 + Win32)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Resource.h"
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
            io.IniFilename = nullptr;

            const HRSRC r = FindResourceW(
                nullptr, MAKEINTRESOURCEW(IDR_FONT_PIXEL_OPERATOR), RT_RCDATA);
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            io.Fonts->AddFontFromMemoryTTF(
                LockResource(LoadResource(nullptr, r)),
                int(SizeofResource(nullptr, r)), 16.0f, &cfg);

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
            const Palette& p = Theme();
            auto& s = ImGui::GetStyle();
            s.WindowBorderSize = 0.0f;
            s.FrameRounding = 6.0f;
            s.PopupRounding = 6.0f;
            s.FramePadding = { 10, 6 };
            s.ItemSpacing = { 8, 8 };

            auto* c = s.Colors;
            c[ImGuiCol_Text] = p.textPrimary;
            c[ImGuiCol_Border] = p.border;
            c[ImGuiCol_FrameBg] = p.surface;
            c[ImGuiCol_FrameBgHovered] = p.surfaceHover;
            c[ImGuiCol_FrameBgActive] = p.surfaceActive;
        }

        bool m_init = false;
        wrl::ComPtr<ID3D11DeviceContext>    m_ctx;
        wrl::ComPtr<ID3D11RenderTargetView> m_rtv;
    };

} // namespace Spectrum

#endif