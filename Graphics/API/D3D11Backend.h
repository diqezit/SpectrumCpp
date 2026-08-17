#ifndef SPECTRUM_CPP_D3D11_BACKEND_H
#define SPECTRUM_CPP_D3D11_BACKEND_H

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"

#include <d3d11.h>
#include <dxgi.h>

namespace Spectrum {

    class D3D11Backend final {
    public:
        D3D11Backend() = default;
        ~D3D11Backend() noexcept { Shutdown(); }

        D3D11Backend(const D3D11Backend&) = delete;
        D3D11Backend& operator=(const D3D11Backend&) = delete;

        [[nodiscard]] bool Initialize(HWND hwnd) {
            Shutdown();
            m_hwnd = hwnd;
            if (const auto rc = Helpers::Window::GetClientRect(hwnd)) {
                m_width = rc->Width();
                m_height = rc->Height();
            }
            return Create() && BindRTV();
        }

        void Shutdown() noexcept {
            if (m_context) {
                m_context->ClearState();
                m_context->Flush();
            }
            m_rtv.Reset();
            m_swapChain.Reset();
            m_context.Reset();
            m_device.Reset();
            m_hwnd = nullptr;
            m_width = m_height = 1;
        }

        [[nodiscard]] bool Resize(int w, int h) {
            if (w == m_width && h == m_height)
                return true;

            m_context->OMSetRenderTargets(0, nullptr, nullptr);
            m_rtv.Reset();
            if (FAILED(m_swapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0)))
                return false;

            m_width = w;
            m_height = h;
            return BindRTV();
        }

        void Clear(const Color& c) {
            const float v[] = { c.r, c.g, c.b, c.a };
            m_context->ClearRenderTargetView(m_rtv.Get(), v);
        }

        [[nodiscard]] bool Present() { return SUCCEEDED(m_swapChain->Present(1, 0)); }

        [[nodiscard]] ID3D11Device* GetD3D11Device() const noexcept { return m_device.Get(); }
        [[nodiscard]] ID3D11DeviceContext* GetD3D11DeviceContext() const noexcept { return m_context.Get(); }
        [[nodiscard]] ID3D11RenderTargetView* GetD3D11RenderTargetView() const noexcept { return m_rtv.Get(); }
        [[nodiscard]] int GetWidth() const noexcept { return m_width; }
        [[nodiscard]] int GetHeight() const noexcept { return m_height; }

    private:
        bool Create() {
            UINT flags = 0;
#ifdef _DEBUG
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            constexpr D3D_FEATURE_LEVEL levels[] = {
                D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
            };

            D3D_FEATURE_LEVEL unused{};
            auto create = [&](UINT f) {
                return D3D11CreateDevice(
                    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, f,
                    levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                    &m_device, &unused, &m_context);
                };

            HRESULT hr = create(flags);
#ifdef _DEBUG
            if (FAILED(hr))
                hr = create(0);
#endif
            if (FAILED(hr))
                return false;

            wrl::ComPtr<IDXGIDevice> dxgi;
            wrl::ComPtr<IDXGIAdapter> adapter;
            wrl::ComPtr<IDXGIFactory> factory;
            m_device.As(&dxgi);
            dxgi->GetAdapter(&adapter);
            adapter->GetParent(IID_PPV_ARGS(&factory));

            DXGI_SWAP_CHAIN_DESC desc{};
            desc.BufferDesc = { UINT(m_width), UINT(m_height), { 60, 1 }, DXGI_FORMAT_R8G8B8A8_UNORM };
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.OutputWindow = m_hwnd;
            desc.Windowed = TRUE;
            desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            return SUCCEEDED(factory->CreateSwapChain(m_device.Get(), &desc, &m_swapChain));
        }

        bool BindRTV() {
            wrl::ComPtr<ID3D11Texture2D> bb;
            m_swapChain->GetBuffer(0, IID_PPV_ARGS(&bb));
            m_device->CreateRenderTargetView(bb.Get(), nullptr, &m_rtv);
            m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);

            const D3D11_VIEWPORT vp{ 0.0f, 0.0f, float(m_width), float(m_height), 0.0f, 1.0f };
            m_context->RSSetViewports(1, &vp);
            return true;
        }

        HWND m_hwnd = nullptr;
        int  m_width = 1;
        int  m_height = 1;

        wrl::ComPtr<ID3D11Device>           m_device;
        wrl::ComPtr<ID3D11DeviceContext>    m_context;
        wrl::ComPtr<IDXGISwapChain>         m_swapChain;
        wrl::ComPtr<ID3D11RenderTargetView> m_rtv;
    };

} // namespace Spectrum

#endif