#ifndef SPECTRUM_CPP_IMGUI_CONTEXT_H
#define SPECTRUM_CPP_IMGUI_CONTEXT_H

#include "Common/Common.h"
#include <d3d11.h>

namespace Spectrum {

    class ImGuiContext final
    {
    public:
        ImGuiContext() = default;
        ~ImGuiContext() noexcept;

        ImGuiContext(const ImGuiContext&) = delete;
        ImGuiContext& operator=(const ImGuiContext&) = delete;

        bool Initialize(
            HWND hwnd,
            ID3D11Device* device,
            ID3D11DeviceContext* deviceContext
        );

        void Shutdown();

        void NewFrame();
        void Render();

        bool ProcessMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

        void SetRenderTargetView(ID3D11RenderTargetView* rtv);

        [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }

    private:
        void SetupStyle();

        bool m_initialized = false;
        wrl::ComPtr<ID3D11DeviceContext> m_deviceContext;
        wrl::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_IMGUI_CONTEXT_H