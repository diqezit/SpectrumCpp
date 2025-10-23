// ImGuiContext.cpp
#include "UI/Core/ImGuiContext.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
);

namespace Spectrum {

    ImGuiContext::~ImGuiContext() noexcept
    {
        Shutdown();
    }

    bool ImGuiContext::Initialize(
        HWND hwnd,
        ID3D11Device* device,
        ID3D11DeviceContext* deviceContext
    )
    {
        if (m_initialized)
        {
            LOG_WARNING("ImGuiContext: Already initialized");
            return true;
        }

        if (!hwnd || !device || !deviceContext)
        {
            LOG_ERROR("ImGuiContext: Invalid parameters");
            return false;
        }

        m_deviceContext = deviceContext;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;  // Disable imgui.ini - prevents state issues

        if (!ImGui_ImplWin32_Init(hwnd))
        {
            LOG_ERROR("ImGuiContext: Failed to initialize Win32 backend");
            ImGui::DestroyContext();
            return false;
        }

        if (!ImGui_ImplDX11_Init(device, deviceContext))
        {
            LOG_ERROR("ImGuiContext: Failed to initialize DX11 backend");
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        SetupStyle();

        m_initialized = true;
        LOG_INFO("ImGuiContext: Initialized successfully");
        return true;
    }

    void ImGuiContext::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_deviceContext.Reset();
        m_renderTargetView.Reset();

        m_initialized = false;
        LOG_INFO("ImGuiContext: Shutdown complete");
    }

    void ImGuiContext::NewFrame()
    {
        if (!m_initialized)
        {
            return;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiContext::Render()
    {
        if (!m_initialized)
        {
            return;
        }

        ImGui::Render();

        auto* drawData = ImGui::GetDrawData();
        if (drawData && m_deviceContext && m_renderTargetView)
        {
            m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
            ImGui_ImplDX11_RenderDrawData(drawData);
        }
    }

    bool ImGuiContext::ProcessMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        if (!m_initialized)
        {
            return false;
        }

        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
    }

    void ImGuiContext::SetRenderTargetView(ID3D11RenderTargetView* rtv)
    {
        m_renderTargetView = rtv;
    }

    void ImGuiContext::SetupStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::StyleColorsDark();

        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(8.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 8.0f);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.95f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.45f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.25f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.45f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.55f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 0.65f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.60f, 0.80f, 1.00f);
    }

} // namespace Spectrum