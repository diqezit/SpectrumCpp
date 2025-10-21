// RenderEngine.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the RenderEngine facade for the rendering system.
//
// Implementation details:
// - Manages Direct2D/DirectWrite resource lifecycle
// - Handles device lost scenarios gracefully (D2DERR_RECREATE_TARGET)
// - Creates and provides Canvas facade for drawing operations
// - Uses D2DHelpers for validation, sanitization, and HRESULT checking
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RenderEngine.h"
#include "D2DHelpers.h"

namespace Spectrum {

    using namespace D2DHelpers;

    namespace {

        [[nodiscard]] D2D1_SIZE_U GetClientSize(HWND hwnd) noexcept
        {
            RECT rc{};
            GetClientRect(hwnd, &rc);
            return ToD2DSizeU(
                static_cast<UINT32>(rc.right - rc.left),
                static_cast<UINT32>(rc.bottom - rc.top)
            );
        }

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    RenderEngine::RenderEngine(HWND hwnd, bool isOverlay)
        : m_hwnd(hwnd)
        , m_isOverlay(isOverlay)
        , m_width(0)
        , m_height(0)
    {
        if (m_hwnd) {
            const auto size = GetClientSize(m_hwnd);
            m_width = size.width;
            m_height = size.height;
        }
    }

    RenderEngine::~RenderEngine()
    {
        DiscardDeviceResources();
    }

    bool RenderEngine::Initialize()
    {
        if (!CreateD2DFactory()) {
            LOG_ERROR("Failed to create D2D factory");
            return false;
        }

        if (!CreateDWriteFactory()) {
            LOG_ERROR("Failed to create DWrite factory");
            return false;
        }

        if (!CreateDeviceResources()) {
            LOG_ERROR("Failed to create device resources");
            return false;
        }

        // Initialize all sub-components (worker classes)
        m_resourceCache = std::make_unique<ResourceCache>(
            m_d2dFactory.Get(),
            m_renderTarget.Get()
        );

        m_geometryBuilder = std::make_unique<GeometryBuilder>(
            m_d2dFactory.Get()
        );

        m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(
            m_renderTarget.Get(),
            m_solidBrush.Get(),
            m_geometryBuilder.get()
        );

        m_gradientRenderer = std::make_unique<GradientRenderer>(
            m_renderTarget.Get(),
            m_solidBrush.Get(),
            m_resourceCache.get(),
            m_geometryBuilder.get()
        );

        m_textRenderer = std::make_unique<TextRenderer>(
            m_renderTarget.Get(),
            m_writeFactory.Get(),
            m_solidBrush.Get()
        );

        m_effectsRenderer = std::make_unique<EffectsRenderer>(
            m_renderTarget.Get(),
            m_solidBrush.Get()
        );

        m_transformManager = std::make_unique<TransformManager>(
            m_renderTarget.Get()
        );

        m_spectrumRenderer = std::make_unique<SpectrumRenderer>(
            m_primitiveRenderer.get(),
            m_gradientRenderer.get(),
            m_geometryBuilder.get()
        );

        // Create the Canvas facade, injecting all dependencies
        m_canvas = std::make_unique<Canvas>(
            m_renderTarget.Get(),
            m_primitiveRenderer.get(),
            m_gradientRenderer.get(),
            m_textRenderer.get(),
            m_effectsRenderer.get(),
            m_transformManager.get(),
            m_spectrumRenderer.get()
        );

        return true;
    }

    void RenderEngine::Resize(int width, int height)
    {
        m_width = width;
        m_height = height;

        if (!m_renderTarget) return;

        const HRESULT hr = m_renderTarget->Resize(
            ToD2DSizeU(
                static_cast<UINT32>(width),
                static_cast<UINT32>(height)
            )
        );

        if (FAILED(hr)) {
            LOG_ERROR("Render target resize failed, discarding device resources");
            DiscardDeviceResources();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Control
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RenderEngine::BeginDraw()
    {
        if (!m_renderTarget) {
            if (CreateDeviceResources()) {
                UpdateComponentsRenderTarget();
            }
        }

        if (m_renderTarget) {
            m_renderTarget->BeginDraw();
        }
    }

    HRESULT RenderEngine::EndDraw()
    {
        if (!m_renderTarget) return S_OK;

        const HRESULT hr = m_renderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET) {
            LOG_WARNING("Device lost, recreating resources");
            DiscardDeviceResources();
        }
        else if (FAILED(hr)) {
            LOG_ERROR("EndDraw failed with HRESULT: 0x" << std::hex << hr);
        }

        return hr;
    }

    void RenderEngine::Clear(const Color& color) const
    {
        if (m_renderTarget) {
            m_renderTarget->Clear(ToD2DColor(color));
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Access
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ID2D1HwndRenderTarget* RenderEngine::GetRenderTarget() const noexcept
    {
        return m_renderTarget.Get();
    }

    int RenderEngine::GetWidth() const noexcept
    {
        return m_width;
    }

    int RenderEngine::GetHeight() const noexcept
    {
        return m_height;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool RenderEngine::CreateD2DFactory()
    {
        const HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );

        return HResult::CheckWithReturn(hr, "D2D1CreateFactory");
    }

    bool RenderEngine::CreateDWriteFactory()
    {
        const HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
        );

        return HResult::CheckWithReturn(hr, "DWriteCreateFactory");
    }

    bool RenderEngine::CreateDeviceResources()
    {
        if (m_renderTarget) return true;

        if (!CreateHwndRenderTarget()) {
            LOG_ERROR("Failed to create HWND render target");
            return false;
        }

        if (!CreateSolidBrush()) {
            LOG_ERROR("Failed to create solid brush");
            return false;
        }

        return true;
    }

    bool RenderEngine::CreateHwndRenderTarget()
    {
        if (!m_hwnd || !m_d2dFactory) return false;

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
            m_hwnd,
            GetClientSize(m_hwnd),
            D2D1_PRESENT_OPTIONS_NONE
        );

        const HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            rtProps,
            hwndProps,
            m_renderTarget.GetAddressOf()
        );

        if (!HResult::CheckWithReturn(hr, "ID2D1Factory::CreateHwndRenderTarget")) {
            return false;
        }

        m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        return true;
    }

    bool RenderEngine::CreateSolidBrush()
    {
        if (!m_renderTarget) return false;

        const HRESULT hr = m_renderTarget->CreateSolidColorBrush(
            ToD2DColor(Color::White()),
            m_solidBrush.GetAddressOf()
        );

        return HResult::CheckWithReturn(hr, "ID2D1RenderTarget::CreateSolidColorBrush");
    }

    void RenderEngine::DiscardDeviceResources()
    {
        // Reset Canvas first (it depends on the workers)
        m_canvas.reset();

        // Reset all worker components
        m_spectrumRenderer.reset();
        m_transformManager.reset();
        m_effectsRenderer.reset();
        m_textRenderer.reset();
        m_gradientRenderer.reset();
        m_primitiveRenderer.reset();
        m_geometryBuilder.reset();
        m_resourceCache.reset();

        // Reset D2D resources
        m_solidBrush.Reset();
        m_renderTarget.Reset();
    }

    void RenderEngine::UpdateComponentsRenderTarget()
    {
        if (!m_renderTarget) return;

        if (m_resourceCache) {
            m_resourceCache->UpdateRenderTarget(m_renderTarget.Get());
        }
        if (m_primitiveRenderer) {
            m_primitiveRenderer->UpdateRenderTarget(m_renderTarget.Get());
        }
        if (m_gradientRenderer) {
            m_gradientRenderer->UpdateRenderTarget(m_renderTarget.Get());
        }
        if (m_textRenderer) {
            m_textRenderer->UpdateRenderTarget(m_renderTarget.Get());
        }
        if (m_effectsRenderer) {
            m_effectsRenderer->UpdateRenderTarget(m_renderTarget.Get());
        }
        if (m_transformManager) {
            m_transformManager->UpdateRenderTarget(m_renderTarget.Get());
        }
    }

} // namespace Spectrum