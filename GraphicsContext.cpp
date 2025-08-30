// GraphicsContext.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// GraphicsContext.cpp: Implementation of the GraphicsContext class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "GraphicsContext.h"
#include "Utils.h"

namespace Spectrum {

    namespace {

        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_SIZE_U ClientSize(HWND hwnd) {
            RECT rc{ 0, 0, 0, 0 };
            GetClientRect(hwnd, &rc);
            return D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        }

        inline bool CreateGradientStops(
            ID2D1RenderTarget* rt,
            const Color& c0,
            const Color& c1,
            wrl::ComPtr<ID2D1GradientStopCollection>& out
        ) {
            D2D1_GRADIENT_STOP stops[2];
            stops[0].color = ToD2DColor(c0);
            stops[0].position = 0.0f;
            stops[1].color = ToD2DColor(c1);
            stops[1].position = 1.0f;

            HRESULT hr = rt->CreateGradientStopCollection(
                stops,
                2,
                D2D1_GAMMA_2_2,
                D2D1_EXTEND_MODE_CLAMP,
                out.GetAddressOf()
            );
            return SUCCEEDED(hr);
        }

        inline D2D1_RENDER_TARGET_PROPERTIES DefaultRTProps() {
            return D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED
                ),
                0,
                0,
                D2D1_RENDER_TARGET_USAGE_NONE,
                D2D1_FEATURE_LEVEL_DEFAULT
            );
        }

    } // anonymous

    GraphicsContext::GraphicsContext(HWND hwnd)
        : m_hwnd(hwnd)
        , m_width(0)
        , m_height(0) {
        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            m_width = rect.right - rect.left;
            m_height = rect.bottom - rect.top;
        }
    }

    GraphicsContext::~GraphicsContext() {
        DiscardDeviceResources();
    }

    bool GraphicsContext::Initialize() {
        HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );
        if (FAILED(hr)) {
            LOG_ERROR("Failed to create D2D factory: " << hr);
            return false;
        }

        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(
                m_writeFactory.GetAddressOf()
                )
        );
        if (FAILED(hr)) {
            LOG_ERROR("Failed to create DWrite factory: " << hr);
            return false;
        }

        return CreateDeviceResources();
    }

    bool GraphicsContext::CreateDeviceResources() {
        if (m_renderTarget) return true;

        D2D1_SIZE_U size = ClientSize(m_hwnd);

        HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            DefaultRTProps(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            m_renderTarget.GetAddressOf()
        );
        if (FAILED(hr)) {
            LOG_ERROR("Failed to create render target: " << hr);
            return false;
        }

        hr = m_renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            m_solidBrush.GetAddressOf()
        );
        if (FAILED(hr)) {
            LOG_ERROR("Failed to create solid brush: " << hr);
            return false;
        }

        if (m_writeFactory) {
            hr = m_writeFactory->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                12.0f,
                L"en-US",
                m_textFormat.GetAddressOf()
            );
            if (FAILED(hr)) {
                LOG_ERROR("Failed to create default text format: " << hr);
                // Non-fatal; text drawing will be limited
            }
        }

        return true;
    }

    void GraphicsContext::DiscardDeviceResources() {
        m_radialBrush.Reset();
        m_linearBrush.Reset();
        m_solidBrush.Reset();
        m_textFormat.Reset();
        m_renderTarget.Reset();
    }

    void GraphicsContext::BeginDraw() {
        if (!m_renderTarget) CreateDeviceResources();
        if (m_renderTarget) m_renderTarget->BeginDraw();
    }

    HRESULT GraphicsContext::EndDraw() {
        if (!m_renderTarget) return S_OK;

        HRESULT hr = m_renderTarget->EndDraw();

        // Recreate target on any failure (not only D2DERR_RECREATE_TARGET)
        if (FAILED(hr)) DiscardDeviceResources();
        return hr;
    }

    void GraphicsContext::Resize(int width, int height) {
        m_width = width;
        m_height = height;
        if (m_renderTarget && width > 0 && height > 0) {
            HRESULT hr = m_renderTarget->Resize(D2D1::SizeU(width, height));
            if (FAILED(hr)) DiscardDeviceResources();
        }
    }

    void GraphicsContext::Clear(const Color& color) {
        if (!m_renderTarget) return;
        m_renderTarget->Clear(ToD2DColor(color));
    }

    ID2D1SolidColorBrush* GraphicsContext::GetSolidBrush(
        const Color& color
    ) {
        if (!m_solidBrush && m_renderTarget) {
            m_renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                m_solidBrush.GetAddressOf()
            );
        }
        if (m_solidBrush)
            m_solidBrush->SetColor(ToD2DColor(color));
        return m_solidBrush.Get();
    }

    ID2D1LinearGradientBrush* GraphicsContext::GetLinearGradientBrush(
        const Color& startColor,
        const Color& endColor,
        const D2D1_POINT_2F& start,
        const D2D1_POINT_2F& end
    ) {
        if (!m_renderTarget) return nullptr;

        wrl::ComPtr<ID2D1GradientStopCollection> stops;
        if (!CreateGradientStops(m_renderTarget.Get(), startColor, endColor, stops))
            return nullptr;

        m_linearBrush.Reset();
        HRESULT hr = m_renderTarget->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(start, end),
            stops.Get(),
            m_linearBrush.GetAddressOf()
        );
        if (FAILED(hr)) {
            LOG_ERROR("Failed to create linear gradient brush: " << hr);
            return nullptr;
        }
        return m_linearBrush.Get();
    }

    ID2D1RadialGradientBrush* GraphicsContext::GetRadialGradientBrush(
        const Color& centerColor,
        const Color& edgeColor,
        const D2D1_POINT_2F& center,
        float radius
    ) {
        if (!m_renderTarget) return nullptr;

        wrl::ComPtr<ID2D1GradientStopCollection> stops;
        if (!CreateGradientStops(m_renderTarget.Get(), centerColor, edgeColor, stops))
            return nullptr;

        m_radialBrush.Reset();
        HRESULT hr = m_renderTarget->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(center, D2D1::Point2F(0, 0), radius, radius),
            stops.Get(),
            m_radialBrush.GetAddressOf()
        );
        if (FAILED(hr)) {
            LOG_ERROR("Failed to create radial gradient brush: " << hr);
            return nullptr;
        }
        return m_radialBrush.Get();
    }

    void GraphicsContext::DrawRectangle(
        const Rect& rect,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget) return;

        D2D1_RECT_F r = D2D1::RectF(
            rect.x,
            rect.y,
            rect.x + rect.width,
            rect.y + rect.height
        );

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        if (filled) m_renderTarget->FillRectangle(&r, b);
        else        m_renderTarget->DrawRectangle(&r, b, strokeWidth);
    }

    void GraphicsContext::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget) return;

        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
            D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height),
            radius,
            radius
        );

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        if (filled) m_renderTarget->FillRoundedRectangle(&rr, b);
        else        m_renderTarget->DrawRoundedRectangle(&rr, b, strokeWidth);
    }

    void GraphicsContext::DrawCircle(
        const Point& center,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        DrawEllipse(center, radius, radius, color, filled, strokeWidth);
    }

    void GraphicsContext::DrawEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget) return;

        D2D1_ELLIPSE e = D2D1::Ellipse(
            D2D1::Point2F(center.x, center.y),
            radiusX,
            radiusY
        );

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        if (filled) m_renderTarget->FillEllipse(&e, b);
        else        m_renderTarget->DrawEllipse(&e, b, strokeWidth);
    }

    void GraphicsContext::DrawLine(
        const Point& start,
        const Point& end,
        const Color& color,
        float strokeWidth
    ) {
        if (!m_renderTarget) return;

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        m_renderTarget->DrawLine(
            D2D1::Point2F(start.x, start.y),
            D2D1::Point2F(end.x, end.y),
            b,
            strokeWidth
        );
    }

    void GraphicsContext::DrawPolyline(
        const std::vector<Point>& points,
        const Color& color,
        float strokeWidth
    ) {
        if (!m_renderTarget || points.size() < 2) return;

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        for (size_t i = 0; i + 1 < points.size(); ++i) {
            m_renderTarget->DrawLine(
                D2D1::Point2F(points[i].x, points[i].y),
                D2D1::Point2F(points[i + 1].x, points[i + 1].y),
                b,
                strokeWidth
            );
        }
    }

    void GraphicsContext::DrawPolygon(
        const std::vector<Point>& points,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget || points.size() < 3) return;

        wrl::ComPtr<ID2D1PathGeometry> geo;
        HRESULT hr = m_d2dFactory->CreatePathGeometry(geo.GetAddressOf());
        if (FAILED(hr)) {
            LOG_ERROR("CreatePathGeometry failed: " << hr);
            return;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geo->Open(sink.GetAddressOf());
        if (FAILED(hr)) {
            LOG_ERROR("Open geometry sink failed: " << hr);
            return;
        }

        sink->BeginFigure(
            D2D1::Point2F(points[0].x, points[0].y),
            filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW
        );
        for (size_t i = 1; i < points.size(); ++i)
            sink->AddLine(D2D1::Point2F(points[i].x, points[i].y));

        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        hr = sink->Close();
        if (FAILED(hr)) {
            LOG_ERROR("Close geometry sink failed: " << hr);
            return;
        }

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        if (filled) m_renderTarget->FillGeometry(geo.Get(), b);
        else        m_renderTarget->DrawGeometry(geo.Get(), b, strokeWidth);
    }

    void GraphicsContext::DrawBezier(
        const Point& start,
        const Point& control1,
        const Point& control2,
        const Point& end,
        const Color& color,
        float strokeWidth
    ) {
        if (!m_renderTarget) return;

        wrl::ComPtr<ID2D1PathGeometry> geo;
        HRESULT hr = m_d2dFactory->CreatePathGeometry(geo.GetAddressOf());
        if (FAILED(hr)) {
            LOG_ERROR("CreatePathGeometry failed: " << hr);
            return;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geo->Open(sink.GetAddressOf());
        if (FAILED(hr)) {
            LOG_ERROR("Open geometry sink failed: " << hr);
            return;
        }

        sink->BeginFigure(D2D1::Point2F(start.x, start.y), D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddBezier(D2D1::BezierSegment(
            D2D1::Point2F(control1.x, control1.y),
            D2D1::Point2F(control2.x, control2.y),
            D2D1::Point2F(end.x, end.y)
        ));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);

        hr = sink->Close();
        if (FAILED(hr)) {
            LOG_ERROR("Close geometry sink failed: " << hr);
            return;
        }

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b) return;

        m_renderTarget->DrawGeometry(geo.Get(), b, strokeWidth);
    }

    void GraphicsContext::DrawGradientRectangle(
        const Rect& rect,
        const Color& startColor,
        const Color& endColor,
        bool horizontal
    ) {
        if (!m_renderTarget) return;

        D2D1_POINT_2F s = D2D1::Point2F(rect.x, rect.y);
        D2D1_POINT_2F e = horizontal
            ? D2D1::Point2F(rect.x + rect.width, rect.y)
            : D2D1::Point2F(rect.x, rect.y + rect.height);

        ID2D1LinearGradientBrush* gb =
            GetLinearGradientBrush(startColor, endColor, s, e);
        if (!gb) return;

        D2D1_RECT_F r = D2D1::RectF(
            rect.x, rect.y, rect.x + rect.width, rect.y + rect.height
        );
        m_renderTarget->FillRectangle(&r, gb);
    }

    void GraphicsContext::DrawRadialGradient(
        const Point& center,
        float radius,
        const Color& centerColor,
        const Color& edgeColor
    ) {
        if (!m_renderTarget) return;

        ID2D1RadialGradientBrush* gb =
            GetRadialGradientBrush(centerColor, edgeColor,
                D2D1::Point2F(center.x, center.y), radius);
        if (!gb) return;

        D2D1_ELLIPSE e = D2D1::Ellipse(
            D2D1::Point2F(center.x, center.y),
            radius,
            radius
        );
        m_renderTarget->FillEllipse(&e, gb);
    }

    void GraphicsContext::DrawText(
        const std::wstring& text,
        const Point& position,
        const Color& color,
        float fontSize
    ) {
        if (!m_renderTarget || text.empty() || !m_writeFactory) return;

        wrl::ComPtr<IDWriteTextFormat> tf = m_textFormat;
        if (!tf || fontSize > 0.0f) {
            wrl::ComPtr<IDWriteTextFormat> temp;
            HRESULT hr = m_writeFactory->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                fontSize > 0.0f ? fontSize : 12.0f,
                L"en-US",
                temp.GetAddressOf()
            );
            if (SUCCEEDED(hr)) tf = temp;
        }

        ID2D1SolidColorBrush* b = GetSolidBrush(color);
        if (!b || !tf) return;

        D2D1_RECT_F layout = D2D1::RectF(
            position.x, position.y,
            position.x + 1000.0f, position.y + 100.0f
        );

        m_renderTarget->DrawText(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            tf.Get(),
            &layout,
            b
        );
    }

    void GraphicsContext::PushTransform(const D2D1_MATRIX_3X2_F& transform) {
        D2D1_MATRIX_3X2_F current;
        m_renderTarget->GetTransform(&current);
        m_transformStack.push_back(current);
        m_renderTarget->SetTransform(transform * current);
    }

    void GraphicsContext::PopTransform() {
        if (m_transformStack.empty()) return;
        m_renderTarget->SetTransform(m_transformStack.back());
        m_transformStack.pop_back();
    }

    void GraphicsContext::SetTransform(const D2D1_MATRIX_3X2_F& transform) {
        if (m_renderTarget) m_renderTarget->SetTransform(transform);
    }

    void GraphicsContext::ResetTransform() {
        if (m_renderTarget)
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    }

} // namespace Spectrum