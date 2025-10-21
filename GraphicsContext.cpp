// GraphicsContext.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the GraphicsContext facade for the rendering system.
//
// Implementation details:
// - Manages Direct2D/DirectWrite resource lifecycle
// - Handles device lost scenarios gracefully (D2DERR_RECREATE_TARGET)
// - Delegates all drawing operations to specialized components
// - Uses D2DHelpers for validation, sanitization, and HRESULT checking
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GraphicsContext.h"
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

    GraphicsContext::GraphicsContext(HWND hwnd)
        : m_hwnd(hwnd)
        , m_width(0)
        , m_height(0)
    {
        if (m_hwnd) {
            const auto size = GetClientSize(m_hwnd);
            m_width = size.width;
            m_height = size.height;
        }
    }

    GraphicsContext::~GraphicsContext()
    {
        DiscardDeviceResources();
    }

    bool GraphicsContext::Initialize()
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

        return true;
    }

    void GraphicsContext::Resize(int width, int height)
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

    void GraphicsContext::BeginDraw()
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

    HRESULT GraphicsContext::EndDraw()
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

    void GraphicsContext::Clear(const Color& color) const
    {
        if (m_renderTarget) {
            m_renderTarget->Clear(ToD2DColor(color));
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Primitives
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawRectangle(
        const Rect& rect,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRectangle(rect, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRoundedRectangle(rect, radius, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawCircle(
        const Point& center,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawCircle(center, radius, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawEllipse(center, radiusX, radiusY, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawLine(
        const Point& start,
        const Point& end,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawLine(start, end, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawPolyline(
        const std::vector<Point>& points,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawPolyline(points, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawPolygon(
        const std::vector<Point>& points,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawPolygon(points, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawArc(center, radius, startAngle, sweepAngle, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Color& color
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRing(center, innerRadius, outerRadius, color);
        }
    }

    void GraphicsContext::DrawSector(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Color& color,
        bool filled
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawSector(center, radius, startAngle, sweepAngle, color, filled);
        }
    }

    void GraphicsContext::DrawRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRegularPolygon(center, radius, sides, rotation, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawStar(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points,
        const Color& color,
        bool filled
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawStar(center, outerRadius, innerRadius, points, color, filled);
        }
    }

    void GraphicsContext::DrawGrid(
        const Rect& bounds,
        int rows,
        int cols,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawGrid(bounds, rows, cols, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawCircleBatch(
        const std::vector<Point>& centers,
        float radius,
        const Color& color,
        bool filled
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawCircleBatch(centers, radius, color, filled);
        }
    }

    void GraphicsContext::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Color& color,
        bool filled
    ) const
    {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRectangleBatch(rects, color, filled);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradients
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawGradientRectangle(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool horizontal
    ) const
    {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawGradientRectangle(rect, stops, horizontal);
        }
    }

    void GraphicsContext::DrawRadialGradient(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) const
    {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawRadialGradient(center, radius, stops);
        }
    }

    void GraphicsContext::DrawGradientCircle(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool filled
    ) const
    {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawGradientCircle(center, radius, stops, filled);
        }
    }

    void GraphicsContext::DrawGradientPath(
        const std::vector<Point>& points,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float strokeWidth
    ) const
    {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawGradientPath(points, stops, strokeWidth);
        }
    }

    void GraphicsContext::DrawAngularGradient(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle,
        const Color& startColor,
        const Color& endColor
    ) const
    {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawAngularGradient(center, radius, startAngle, endAngle, startColor, endColor);
        }
    }

    void GraphicsContext::DrawVerticalGradientBar(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float cornerRadius
    ) const
    {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawVerticalGradientBar(rect, stops, cornerRadius);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Effects
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawWithShadow(
        std::function<void()> drawCallback,
        const Point& offset,
        float blur,
        const Color& shadowColor
    ) const
    {
        if (m_effectsRenderer) {
            m_effectsRenderer->DrawWithShadow(drawCallback, offset, blur, shadowColor);
        }
    }

    void GraphicsContext::DrawGlow(
        const Point& center,
        float radius,
        const Color& glowColor,
        float intensity
    ) const
    {
        if (m_effectsRenderer) {
            m_effectsRenderer->DrawGlow(center, radius, glowColor, intensity);
        }
    }

    void GraphicsContext::BeginOpacityLayer(float opacity) const
    {
        if (m_effectsRenderer) {
            m_effectsRenderer->BeginOpacityLayer(opacity);
        }
    }

    void GraphicsContext::EndOpacityLayer() const
    {
        if (m_effectsRenderer) {
            m_effectsRenderer->EndOpacityLayer();
        }
    }

    void GraphicsContext::PushClipRect(const Rect& rect) const
    {
        if (m_effectsRenderer) {
            m_effectsRenderer->PushClipRect(rect);
        }
    }

    void GraphicsContext::PopClipRect() const
    {
        if (m_effectsRenderer) {
            m_effectsRenderer->PopClipRect();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transforms
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::PushTransform() const
    {
        if (m_transformManager) {
            m_transformManager->PushTransform();
        }
    }

    void GraphicsContext::PopTransform() const
    {
        if (m_transformManager) {
            m_transformManager->PopTransform();
        }
    }

    void GraphicsContext::RotateAt(const Point& center, float angleDegrees) const
    {
        if (m_transformManager) {
            m_transformManager->RotateAt(center, angleDegrees);
        }
    }

    void GraphicsContext::ScaleAt(const Point& center, float scaleX, float scaleY) const
    {
        if (m_transformManager) {
            m_transformManager->ScaleAt(center, scaleX, scaleY);
        }
    }

    void GraphicsContext::TranslateBy(float dx, float dy) const
    {
        if (m_transformManager) {
            m_transformManager->TranslateBy(dx, dy);
        }
    }

    void GraphicsContext::SetTransform(const D2D1_MATRIX_3X2_F& transform) const
    {
        if (m_transformManager) {
            m_transformManager->SetTransform(transform);
        }
    }

    void GraphicsContext::ResetTransform() const
    {
        if (m_transformManager) {
            m_transformManager->ResetTransform();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawText(
        const std::wstring& text,
        const Point& position,
        const Color& color,
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) const
    {
        if (m_textRenderer) {
            m_textRenderer->DrawText(text, position, color, fontSize, alignment);
        }
    }

    void GraphicsContext::DrawTextWithOutline(
        const std::wstring& text,
        const Point& position,
        const Color& fillColor,
        const Color& outlineColor,
        float fontSize,
        float outlineWidth
    ) const
    {
        if (m_textRenderer) {
            m_textRenderer->DrawTextWithOutline(text, position, fillColor, outlineColor, fontSize, outlineWidth);
        }
    }

    void GraphicsContext::DrawTextRotated(
        const std::wstring& text,
        const Point& position,
        float angleDegrees,
        const Color& color,
        float fontSize
    ) const
    {
        if (m_textRenderer) {
            m_textRenderer->DrawTextRotated(text, position, angleDegrees, color, fontSize);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Visualization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawSpectrumBars(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const BarStyle& style,
        const Color& color
    ) const
    {
        if (m_spectrumRenderer) {
            m_spectrumRenderer->DrawSpectrumBars(spectrum, bounds, style, color);
        }
    }

    void GraphicsContext::DrawWaveform(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& color,
        float strokeWidth,
        bool mirror
    ) const
    {
        if (m_spectrumRenderer) {
            m_spectrumRenderer->DrawWaveform(spectrum, bounds, color, strokeWidth, mirror);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Access
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ID2D1HwndRenderTarget* GraphicsContext::GetRenderTarget() const noexcept
    {
        return m_renderTarget.Get();
    }

    int GraphicsContext::GetWidth() const noexcept
    {
        return m_width;
    }

    int GraphicsContext::GetHeight() const noexcept
    {
        return m_height;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool GraphicsContext::CreateD2DFactory()
    {
        const HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );

        return HResult::CheckWithReturn(hr, "D2D1CreateFactory");
    }

    bool GraphicsContext::CreateDWriteFactory()
    {
        const HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
        );

        return HResult::CheckWithReturn(hr, "DWriteCreateFactory");
    }

    bool GraphicsContext::CreateDeviceResources()
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

    bool GraphicsContext::CreateHwndRenderTarget()
    {
        if (!m_hwnd || !m_d2dFactory) return false;

        const HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(
                m_hwnd,
                GetClientSize(m_hwnd)
            ),
            m_renderTarget.GetAddressOf()
        );

        if (!HResult::CheckWithReturn(hr, "ID2D1Factory::CreateHwndRenderTarget")) {
            return false;
        }

        m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        return true;
    }

    bool GraphicsContext::CreateSolidBrush()
    {
        if (!m_renderTarget) return false;

        const HRESULT hr = m_renderTarget->CreateSolidColorBrush(
            ToD2DColor(Color::White()),
            m_solidBrush.GetAddressOf()
        );

        return HResult::CheckWithReturn(hr, "ID2D1RenderTarget::CreateSolidColorBrush");
    }

    void GraphicsContext::DiscardDeviceResources()
    {
        m_spectrumRenderer.reset();
        m_transformManager.reset();
        m_effectsRenderer.reset();
        m_textRenderer.reset();
        m_gradientRenderer.reset();
        m_primitiveRenderer.reset();
        m_geometryBuilder.reset();
        m_resourceCache.reset();

        m_solidBrush.Reset();
        m_renderTarget.Reset();
    }

    void GraphicsContext::UpdateComponentsRenderTarget()
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