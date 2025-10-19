// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the GraphicsContext, the central facade for the
// entire rendering system. It manages D2D/DWrite resources and delegates
// all drawing calls to specialized sub-components
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GraphicsContext.h"
#include "StringUtils.h"

namespace Spectrum {

    namespace {
        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_SIZE_U GetClientSize(HWND hwnd) {
            RECT rc{};
            GetClientRect(hwnd, &rc);
            return D2D1::SizeU(
                static_cast<UINT32>(rc.right - rc.left),
                static_cast<UINT32>(rc.bottom - rc.top)
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // CONSTRUCTOR & DESTRUCTOR
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GraphicsContext::GraphicsContext(HWND hwnd)
        : m_hwnd(hwnd)
        , m_width(0)
        , m_height(0)
    {
        if (m_hwnd) {
            auto size = GetClientSize(m_hwnd);
            m_width = size.width;
            m_height = size.height;
        }
    }

    GraphicsContext::~GraphicsContext() {
        DiscardDeviceResources();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // INITIALIZATION & LIFECYCLE
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool GraphicsContext::Initialize() {
        if (!CreateD2DFactory()) {
            return false;
        }
        if (!CreateDWriteFactory()) {
            return false;
        }
        if (!CreateDeviceResources()) {
            return false;
        }

        // create all sub-components in correct dependency order
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

    // single threaded factory for performance
    // user expects smooth animation so we avoid multithread overhead
    bool GraphicsContext::CreateD2DFactory() {
        return SUCCEEDED(
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                m_d2dFactory.GetAddressOf()
            )
        );
    }

    // shared factory allows text resources to be shared across app
    bool GraphicsContext::CreateDWriteFactory() {
        return SUCCEEDED(
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
            )
        );
    }

    bool GraphicsContext::CreateDeviceResources() {
        if (m_renderTarget) {
            return true;
        }
        if (!CreateHwndRenderTarget()) {
            return false;
        }
        return CreateSolidBrush();
    }

    // create render target for window
    // set antialiasing for smooth visuals
    bool GraphicsContext::CreateHwndRenderTarget() {
        if (!m_hwnd || !m_d2dFactory) {
            return false;
        }

        HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(
                m_hwnd,
                GetClientSize(m_hwnd)
            ),
            m_renderTarget.GetAddressOf()
        );

        if (FAILED(hr)) {
            return false;
        }

        // user expects sharp text and smooth shapes
        m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        return true;
    }

    // create single brush for solid colors
    // its color will be updated before each draw call
    bool GraphicsContext::CreateSolidBrush() {
        if (!m_renderTarget) {
            return false;
        }
        return SUCCEEDED(
            m_renderTarget->CreateSolidColorBrush(
                ToD2DColor(Color::White()),
                m_solidBrush.GetAddressOf()
            )
        );
    }

    // release all device-dependent resources
    // must be called when device is lost (e.g. display driver update)
    void GraphicsContext::DiscardDeviceResources() {
        // release components in reverse order of creation
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

    // update render target pointer in all sub-components after device is recreated
    void GraphicsContext::UpdateComponentsRenderTarget() {
        if (!m_renderTarget) {
            return;
        }

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

    // on window resize, update render target size
    // if this fails, device is lost and we must recreate everything
    void GraphicsContext::Resize(int width, int height) {
        m_width = width;
        m_height = height;

        if (m_renderTarget) {
            if (FAILED(m_renderTarget->Resize(
                D2D1::SizeU(
                    static_cast<UINT32>(width),
                    static_cast<UINT32>(height)
                )
            ))) {
                DiscardDeviceResources();
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // CORE DRAWING LOOP
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    // prepare for drawing
    // if device was lost, try to recreate it
    void GraphicsContext::BeginDraw() {
        if (!m_renderTarget) {
            CreateDeviceResources();
            UpdateComponentsRenderTarget();
        }
        if (m_renderTarget) {
            m_renderTarget->BeginDraw();
        }
    }

    // finish drawing and present frame
    // D2DERR_RECREATE_TARGET means device was lost and needs to be rebuilt
    HRESULT GraphicsContext::EndDraw() {
        if (!m_renderTarget) {
            return S_OK;
        }

        HRESULT hr = m_renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            DiscardDeviceResources();
        }
        return hr;
    }

    void GraphicsContext::Clear(const Color& color) {
        if (m_renderTarget) {
            m_renderTarget->Clear(ToD2DColor(color));
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // PRIMITIVES - DELEGATION TO PrimitiveRenderer
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawRectangle(
        const Rect& rect,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
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
    ) {
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
    ) {
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
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawEllipse(center, radiusX, radiusY, color, filled, strokeWidth);
        }
    }

    void GraphicsContext::DrawLine(
        const Point& start,
        const Point& end,
        const Color& color,
        float strokeWidth
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawLine(start, end, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawPolyline(
        const std::vector<Point>& points,
        const Color& color,
        float strokeWidth
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawPolyline(points, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawPolygon(
        const std::vector<Point>& points,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
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
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawArc(center, radius, startAngle, sweepAngle, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Color& color
    ) {
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
    ) {
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
    ) {
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
    ) {
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
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawGrid(bounds, rows, cols, color, strokeWidth);
        }
    }

    void GraphicsContext::DrawCircleBatch(
        const std::vector<Point>& centers,
        float radius,
        const Color& color,
        bool filled
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawCircleBatch(centers, radius, color, filled);
        }
    }

    void GraphicsContext::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Color& color,
        bool filled
    ) {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRectangleBatch(rects, color, filled);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // GRADIENTS - DELEGATION TO GradientRenderer
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawGradientRectangle(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool horizontal
    ) {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawGradientRectangle(rect, stops, horizontal);
        }
    }

    void GraphicsContext::DrawRadialGradient(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawRadialGradient(center, radius, stops);
        }
    }

    void GraphicsContext::DrawGradientCircle(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool filled
    ) {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawGradientCircle(center, radius, stops, filled);
        }
    }

    void GraphicsContext::DrawGradientPath(
        const std::vector<Point>& points,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float strokeWidth
    ) {
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
    ) {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawAngularGradient(center, radius, startAngle, endAngle, startColor, endColor);
        }
    }

    void GraphicsContext::DrawVerticalGradientBar(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float cornerRadius
    ) {
        if (m_gradientRenderer) {
            m_gradientRenderer->DrawVerticalGradientBar(rect, stops, cornerRadius);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // EFFECTS - DELEGATION TO EffectsRenderer
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawWithShadow(
        std::function<void()> drawCallback,
        const Point& offset,
        float blur,
        const Color& shadowColor
    ) {
        if (m_effectsRenderer) {
            m_effectsRenderer->DrawWithShadow(drawCallback, offset, blur, shadowColor);
        }
    }

    void GraphicsContext::DrawGlow(
        const Point& center,
        float radius,
        const Color& glowColor,
        float intensity
    ) {
        if (m_effectsRenderer) {
            m_effectsRenderer->DrawGlow(center, radius, glowColor, intensity);
        }
    }

    void GraphicsContext::BeginOpacityLayer(float opacity) {
        if (m_effectsRenderer) {
            m_effectsRenderer->BeginOpacityLayer(opacity);
        }
    }

    void GraphicsContext::EndOpacityLayer() {
        if (m_effectsRenderer) {
            m_effectsRenderer->EndOpacityLayer();
        }
    }

    void GraphicsContext::PushClipRect(const Rect& rect) {
        if (m_effectsRenderer) {
            m_effectsRenderer->PushClipRect(rect);
        }
    }

    void GraphicsContext::PopClipRect() {
        if (m_effectsRenderer) {
            m_effectsRenderer->PopClipRect();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // TRANSFORMS - DELEGATION TO TransformManager
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::PushTransform() {
        if (m_transformManager) {
            m_transformManager->PushTransform();
        }
    }

    void GraphicsContext::PopTransform() {
        if (m_transformManager) {
            m_transformManager->PopTransform();
        }
    }

    void GraphicsContext::RotateAt(const Point& center, float angleDegrees) {
        if (m_transformManager) {
            m_transformManager->RotateAt(center, angleDegrees);
        }
    }

    void GraphicsContext::ScaleAt(const Point& center, float scaleX, float scaleY) {
        if (m_transformManager) {
            m_transformManager->ScaleAt(center, scaleX, scaleY);
        }
    }

    void GraphicsContext::TranslateBy(float dx, float dy) {
        if (m_transformManager) {
            m_transformManager->TranslateBy(dx, dy);
        }
    }

    void GraphicsContext::SetTransform(const D2D1_MATRIX_3X2_F& transform) {
        if (m_transformManager) {
            m_transformManager->SetTransform(transform);
        }
    }

    void GraphicsContext::ResetTransform() {
        if (m_transformManager) {
            m_transformManager->ResetTransform();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // TEXT - DELEGATION TO TextRenderer
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawText(
        const std::wstring& text,
        const Point& position,
        const Color& color,
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) {
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
    ) {
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
    ) {
        if (m_textRenderer) {
            m_textRenderer->DrawTextRotated(text, position, angleDegrees, color, fontSize);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SPECTRUM - DELEGATION TO SpectrumRenderer
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GraphicsContext::DrawSpectrumBars(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const BarStyle& style,
        const Color& color
    ) {
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
    ) {
        if (m_spectrumRenderer) {
            m_spectrumRenderer->DrawWaveform(spectrum, bounds, color, strokeWidth, mirror);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // GETTERS
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ID2D1HwndRenderTarget* GraphicsContext::GetRenderTarget() const noexcept {
        return m_renderTarget.Get();
    }

    int GraphicsContext::GetWidth() const noexcept {
        return m_width;
    }

    int GraphicsContext::GetHeight() const noexcept {
        return m_height;
    }

} // namespace Spectrum