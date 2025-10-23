// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GraphicsAPI.cpp - Unified implementation - REFACTORED & OPTIMIZED
//
// IMPROVEMENTS:
// - Eliminated code duplication (gradient creation, canvas delegation)  
// - Thread-safe gradient cache with size limits
// - Better error handling with validation
// - Improved resource management
// - Template helpers to reduce boilerplate
// - Fixed ownership issues (ComPtr returns)
// - Bounds checking for enum conversions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/GraphicsAPI.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <shared_mutex>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // INTERNAL HELPERS - Eliminate duplication
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Internal {

        // Unified PathGeometry creation
        template<typename BuildFunc>
        wrl::ComPtr<ID2D1PathGeometry> CreatePathGeometry(
            ID2D1Factory* factory,
            BuildFunc&& buildFunc,
            const char* context = "Internal"
        ) {
            VALIDATE_PTR_OR_RETURN_VALUE(factory, context, nullptr);

            wrl::ComPtr<ID2D1PathGeometry> geometry;
            wrl::ComPtr<ID2D1GeometrySink> sink;

            if (FAILED(factory->CreatePathGeometry(&geometry)) ||
                FAILED(geometry->Open(&sink))) {
                return nullptr;
            }

            buildFunc(sink.Get());

            if (FAILED(sink->Close())) {
                return nullptr;
            }

            return geometry;
        }

        // Unified gradient stop collection creation
        wrl::ComPtr<ID2D1GradientStopCollection> CreateGradientStopCollection(
            ID2D1RenderTarget* renderTarget,
            const std::vector<GradientStop>& stops,
            float globalAlpha = 1.0f
        ) {
            VALIDATE_PTR_OR_RETURN_VALUE(renderTarget, "CreateGradientStopCollection", nullptr);
            if (stops.empty()) return nullptr;

            std::vector<D2D1_GRADIENT_STOP> d2dStops;
            d2dStops.reserve(stops.size());

            for (const auto& stop : stops) {
                Color color = stop.color;
                color.a *= globalAlpha;
                d2dStops.push_back({ stop.position, Helpers::TypeConversion::ToD2DColor(color) });
            }

            wrl::ComPtr<ID2D1GradientStopCollection> collection;
            renderTarget->CreateGradientStopCollection(
                d2dStops.data(),
                static_cast<UINT32>(d2dStops.size()),
                &collection
            );

            return collection;
        }

        // Unified glow rendering
        template<typename DrawFunc>
        void DrawGlowEffect(
            DrawFunc&& drawFunc,
            const Color& glowColor,
            float intensity,
            int layers
        ) {
            using namespace Constants::Effects;
            layers = Helpers::Math::Clamp(layers, kMinGlowLayers, kMaxGlowLayers);

            for (int i = layers; i > 0; --i) {
                const float t = static_cast<float>(i) / layers;
                const float expansion = i * kGlowExpansionStep;
                const float alpha = (1.0f - t) * kGlowIntensityFactor * intensity * glowColor.a;

                drawFunc(expansion, glowColor.WithAlpha(alpha));
            }
        }

        // Thread-safe gradient brush cache with size limit
        template<typename TBrush>
        class GradientBrushCache {
        public:
            explicit GradientBrushCache(size_t maxSize = Constants::Cache::kMaxGradientBrushes)
                : m_maxSize(maxSize) {
            }

            template<typename CreateFunc>
            wrl::ComPtr<TBrush> GetOrCreate(size_t hash, CreateFunc&& createFunc) {
                {
                    std::shared_lock lock(m_mutex);
                    if (auto it = m_cache.find(hash); it != m_cache.end()) {
                        return it->second;
                    }
                }

                std::unique_lock lock(m_mutex);

                if (auto it = m_cache.find(hash); it != m_cache.end()) {
                    return it->second;
                }

                auto brush = createFunc();
                if (brush && m_cache.size() < m_maxSize) {
                    m_cache[hash] = brush;
                }

                return brush;
            }

            void Clear() {
                std::unique_lock lock(m_mutex);
                m_cache.clear();
            }

        private:
            std::unordered_map<size_t, wrl::ComPtr<TBrush>> m_cache;
            mutable std::shared_mutex m_mutex;
            size_t m_maxSize;
        };

    } // namespace Internal

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paint Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct Paint::Impl {
        BrushType m_brushType = BrushType::Solid;
        Color m_solidColor = Color(1, 1, 1, 1);
        Point m_linearStart, m_linearEnd, m_radialCenter;
        float m_radialRadiusX = 0.0f, m_radialRadiusY = 0.0f;
        std::vector<GradientStop> m_gradientStops;
        PaintStyle m_style = PaintStyle::Fill;
        float m_strokeWidth = 1.0f;
        StrokeCap m_strokeCap = StrokeCap::Flat;
        StrokeJoin m_strokeJoin = StrokeJoin::Miter;
        float m_miterLimit = 10.0f;
        DashStyle m_dashStyle = DashStyle::Solid;
        std::vector<float> m_dashPattern;
        float m_dashOffset = 0.0f;
        float m_globalAlpha = 1.0f;
    };

    Paint::Paint() : m_impl(std::make_unique<Impl>()) {}
    Paint::~Paint() = default;
    Paint::Paint(const Paint& other) : m_impl(std::make_unique<Impl>(*other.m_impl)) {}
    Paint::Paint(Paint&&) noexcept = default;
    Paint& Paint::operator=(const Paint& other) {
        if (this != &other) m_impl = std::make_unique<Impl>(*other.m_impl);
        return *this;
    }
    Paint& Paint::operator=(Paint&&) noexcept = default;

    Paint Paint::Fill(const Color& color) {
        Paint p;
        p.m_impl->m_solidColor = color;
        p.m_impl->m_style = PaintStyle::Fill;
        return p;
    }

    Paint Paint::Stroke(const Color& color, float width) {
        Paint p;
        p.m_impl->m_solidColor = color;
        p.m_impl->m_style = PaintStyle::Stroke;
        p.m_impl->m_strokeWidth = width;
        return p;
    }

    Paint Paint::LinearGradient(const Point& start, const Point& end, const std::vector<GradientStop>& stops) {
        Paint p;
        p.m_impl->m_brushType = BrushType::LinearGradient;
        p.m_impl->m_linearStart = start;
        p.m_impl->m_linearEnd = end;
        p.m_impl->m_gradientStops = stops;
        return p;
    }

    Paint Paint::RadialGradient(const Point& center, float radiusX, float radiusY, const std::vector<GradientStop>& stops) {
        Paint p;
        p.m_impl->m_brushType = BrushType::RadialGradient;
        p.m_impl->m_radialCenter = center;
        p.m_impl->m_radialRadiusX = radiusX;
        p.m_impl->m_radialRadiusY = radiusY;
        p.m_impl->m_gradientStops = stops;
        return p;
    }

    Paint Paint::RadialGradient(const Point& center, float radius, const std::vector<GradientStop>& stops) {
        return RadialGradient(center, radius, radius, stops);
    }

    Paint& Paint::WithStyle(PaintStyle style) {
        m_impl->m_style = style;
        return *this;
    }

    Paint& Paint::WithStrokeCap(StrokeCap cap) {
        m_impl->m_strokeCap = cap;
        return *this;
    }

    Paint& Paint::WithStrokeJoin(StrokeJoin join) {
        m_impl->m_strokeJoin = join;
        return *this;
    }

    Paint& Paint::WithColor(const Color& color) {
        m_impl->m_solidColor = color;
        m_impl->m_brushType = BrushType::Solid;
        return *this;
    }

    Paint& Paint::WithAlpha(float alpha) {
        m_impl->m_globalAlpha = std::clamp(alpha, 0.0f, 1.0f);
        return *this;
    }

    Paint& Paint::WithStrokeWidth(float width) {
        m_impl->m_strokeWidth = std::max(0.0f, width);
        return *this;
    }

    Paint& Paint::WithMiterLimit(float limit) {
        m_impl->m_miterLimit = std::max(0.0f, limit);
        return *this;
    }

    Paint& Paint::WithStrokeOptions(const StrokeOptions& options) {
        m_impl->m_strokeWidth = options.width;
        m_impl->m_strokeCap = options.cap;
        m_impl->m_strokeJoin = options.join;
        m_impl->m_miterLimit = options.miterLimit;
        m_impl->m_dashStyle = options.dashStyle;
        m_impl->m_dashPattern = options.dashPattern;
        m_impl->m_dashOffset = options.dashOffset;
        return *this;
    }

    PaintStyle Paint::GetStyle() const { return m_impl->m_style; }
    BrushType Paint::GetBrushType() const { return m_impl->m_brushType; }
    Color Paint::GetColor() const { return m_impl->m_solidColor; }
    float Paint::GetStrokeWidth() const { return m_impl->m_strokeWidth; }
    float Paint::GetAlpha() const { return m_impl->m_globalAlpha; }
    const std::vector<GradientStop>& Paint::GetGradientStops() const { return m_impl->m_gradientStops; }
    Point Paint::GetLinearStart() const { return m_impl->m_linearStart; }
    Point Paint::GetLinearEnd() const { return m_impl->m_linearEnd; }
    Point Paint::GetRadialCenter() const { return m_impl->m_radialCenter; }
    float Paint::GetRadialRadiusX() const { return m_impl->m_radialRadiusX; }
    float Paint::GetRadialRadiusY() const { return m_impl->m_radialRadiusY; }

    StrokeOptions Paint::GetStrokeOptions() const {
        StrokeOptions options;
        options.width = m_impl->m_strokeWidth;
        options.cap = m_impl->m_strokeCap;
        options.join = m_impl->m_strokeJoin;
        options.miterLimit = m_impl->m_miterLimit;
        options.dashStyle = m_impl->m_dashStyle;
        options.dashPattern = m_impl->m_dashPattern;
        options.dashOffset = m_impl->m_dashOffset;
        return options;
    }

    bool Paint::IsFilled() const {
        return m_impl->m_style == PaintStyle::Fill || m_impl->m_style == PaintStyle::FillAndStroke;
    }

    bool Paint::IsStroked() const {
        return m_impl->m_style == PaintStyle::Stroke || m_impl->m_style == PaintStyle::FillAndStroke;
    }

    bool Paint::IsGradient() const {
        return m_impl->m_brushType != BrushType::Solid;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // GraphicsCore Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct GraphicsCore::Impl {
        wrl::ComPtr<ID2D1Factory> m_d2dFactory;
        wrl::ComPtr<IDWriteFactory> m_dwriteFactory;
        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;
        Helpers::Gdi::AlphaDC m_alphaDC;
        wrl::ComPtr<ID3D11Device> m_d3dDevice;
        wrl::ComPtr<ID3D11DeviceContext> m_d3dContext;
        wrl::ComPtr<IDXGISwapChain> m_swapChain;
        wrl::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        HWND m_hwnd = nullptr;
        int m_width = 1;
        int m_height = 1;
        WindowMode m_windowMode = WindowMode::Normal;
        bool m_isDrawing = false;
        std::stack<D2D1_MATRIX_3X2_F> m_transformStack;
        wrl::ComPtr<ID2D1SolidColorBrush> m_solidBrush;
        Internal::GradientBrushCache<ID2D1LinearGradientBrush> m_linearGradientCache;
        Internal::GradientBrushCache<ID2D1RadialGradientBrush> m_radialGradientCache;

        bool InitializeFactories();
        bool CreateRenderTarget();
        bool CreateD3D11Resources();
        void HandleDeviceLost();
        size_t HashGradientStops(const std::vector<GradientStop>& stops) const noexcept;
    };

    GraphicsCore::TransformScope::TransformScope(GraphicsCore* core)
        : m_core(core), m_active(false) {
        if (m_core) {
            m_core->PushTransform();
            m_active = true;
        }
    }

    GraphicsCore::TransformScope::~TransformScope() noexcept {
        if (m_active && m_core) {
            try { m_core->PopTransform(); }
            catch (...) {}
        }
    }

    GraphicsCore::TransformScope::TransformScope(TransformScope&& other) noexcept
        : m_core(std::exchange(other.m_core, nullptr))
        , m_active(std::exchange(other.m_active, false)) {
    }

    GraphicsCore::TransformScope& GraphicsCore::TransformScope::operator=(TransformScope&& other) noexcept {
        if (this != &other) {
            if (m_active && m_core) {
                try { m_core->PopTransform(); }
                catch (...) {}
            }
            m_core = std::exchange(other.m_core, nullptr);
            m_active = std::exchange(other.m_active, false);
        }
        return *this;
    }

    GraphicsCore::GraphicsCore() : m_impl(std::make_unique<Impl>()) {}
    GraphicsCore::~GraphicsCore() noexcept { try { Shutdown(); } catch (...) {} }

    bool GraphicsCore::InitializeD2D(HWND hwnd, WindowMode mode) {
        VALIDATE_CONDITION_OR_RETURN_FALSE(hwnd && ::IsWindow(hwnd), "Invalid HWND", "GraphicsCore");

        m_impl->m_hwnd = hwnd;
        m_impl->m_windowMode = mode;

        _Analysis_assume_(hwnd != nullptr);

        RECT rc;
        ::GetClientRect(hwnd, &rc);
        m_impl->m_width = std::max(1L, rc.right - rc.left);
        m_impl->m_height = std::max(1L, rc.bottom - rc.top);

        return m_impl->InitializeFactories() && m_impl->CreateRenderTarget();
    }

    bool GraphicsCore::InitializeD3D11(HWND hwnd) {
        VALIDATE_CONDITION_OR_RETURN_FALSE(hwnd && ::IsWindow(hwnd), "Invalid HWND", "GraphicsCore");

        m_impl->m_hwnd = hwnd;

        _Analysis_assume_(hwnd != nullptr);

        RECT rc;
        ::GetClientRect(hwnd, &rc);
        m_impl->m_width = std::max(1L, rc.right - rc.left);
        m_impl->m_height = std::max(1L, rc.bottom - rc.top);

        return m_impl->CreateD3D11Resources();
    }

    void GraphicsCore::Shutdown() noexcept {
        try {
            ClearCache();
            m_impl->m_alphaDC.Reset();
            m_impl->m_renderTarget.Reset();
            m_impl->m_renderTargetView.Reset();
            m_impl->m_swapChain.Reset();
            m_impl->m_d3dContext.Reset();
            m_impl->m_d3dDevice.Reset();
            m_impl->m_dwriteFactory.Reset();
            m_impl->m_d2dFactory.Reset();
        }
        catch (...) {}
    }

    bool GraphicsCore::RecreateResources(int width, int height) {
        using namespace Constants::Rendering;

        width = Helpers::Sanitize::ClampValue(width, kMinSize, kMaxSize);
        height = Helpers::Sanitize::ClampValue(height, kMinSize, kMaxSize);

        m_impl->m_width = width;
        m_impl->m_height = height;

        ClearCache();

        if (m_impl->m_d2dFactory) {
            m_impl->m_renderTarget.Reset();
            m_impl->m_alphaDC.Reset();
            return m_impl->CreateRenderTarget();
        }

        if (m_impl->m_swapChain) {
            m_impl->m_renderTargetView.Reset();
            m_impl->m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);

            if (FAILED(m_impl->m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
                return false;

            wrl::ComPtr<ID3D11Texture2D> backBuffer;
            if (FAILED(m_impl->m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
                return false;

            if (FAILED(m_impl->m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_impl->m_renderTargetView)))
                return false;

            m_impl->m_d3dContext->OMSetRenderTargets(1, m_impl->m_renderTargetView.GetAddressOf(), nullptr);

            D3D11_VIEWPORT vp = { 0, 0, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
            m_impl->m_d3dContext->RSSetViewports(1, &vp);

            return true;
        }

        return false;
    }

    bool GraphicsCore::BeginDraw() {
        if (m_impl->m_isDrawing) return false;
        VALIDATE_PTR_OR_RETURN_FALSE(m_impl->m_renderTarget.Get(), "GraphicsCore");

        if (m_impl->m_windowMode == WindowMode::Overlay && m_impl->m_alphaDC.IsValid()) {
            RECT rc = { 0, 0, m_impl->m_width, m_impl->m_height };
            auto dcTarget = static_cast<ID2D1DCRenderTarget*>(m_impl->m_renderTarget.Get());
            if (FAILED(dcTarget->BindDC(m_impl->m_alphaDC.GetDC(), &rc)))
                return false;
        }

        m_impl->m_renderTarget->BeginDraw();
        m_impl->m_isDrawing = true;
        return true;
    }

    HRESULT GraphicsCore::EndDraw() {
        if (!m_impl->m_isDrawing) return S_OK;

        HRESULT hr = m_impl->m_renderTarget->EndDraw();
        m_impl->m_isDrawing = false;

        if (SUCCEEDED(hr) && m_impl->m_windowMode == WindowMode::Overlay) {
            if (m_impl->m_alphaDC.IsValid()) {
                POINT srcPos = { 0, 0 };
                SIZE wndSize = { m_impl->m_width, m_impl->m_height };
                BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

                if (!::UpdateLayeredWindow(m_impl->m_hwnd, nullptr, nullptr, &wndSize,
                    m_impl->m_alphaDC.GetDC(), &srcPos, 0, &blend, ULW_ALPHA)) {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                }
            }
        }

        if (hr == D2DERR_RECREATE_TARGET) {
            m_impl->HandleDeviceLost();
        }

        return hr;
    }

    void GraphicsCore::Clear(const Color& color) {
        if (m_impl->m_renderTarget) {
            m_impl->m_renderTarget->Clear(Helpers::TypeConversion::ToD2DColor(color));
        }
    }

    bool GraphicsCore::IsDrawing() const noexcept { return m_impl->m_isDrawing; }

    void GraphicsCore::PushTransform() {
        VALIDATE_PTR_OR_RETURN(m_impl->m_renderTarget.Get(), "GraphicsCore");
        D2D1_MATRIX_3X2_F current;
        m_impl->m_renderTarget->GetTransform(&current);
        m_impl->m_transformStack.push(current);
    }

    void GraphicsCore::PopTransform() {
        VALIDATE_PTR_OR_RETURN(m_impl->m_renderTarget.Get(), "GraphicsCore");
        if (!m_impl->m_transformStack.empty()) {
            m_impl->m_renderTarget->SetTransform(m_impl->m_transformStack.top());
            m_impl->m_transformStack.pop();
        }
    }

    void GraphicsCore::Rotate(const Point& center, float degrees) {
        if (m_impl->m_renderTarget) {
            D2D1_MATRIX_3X2_F current;
            m_impl->m_renderTarget->GetTransform(&current);
            m_impl->m_renderTarget->SetTransform(
                D2D1::Matrix3x2F::Rotation(degrees, Helpers::TypeConversion::ToD2DPoint(center)) * current
            );
        }
    }

    void GraphicsCore::Scale(const Point& center, float sx, float sy) {
        if (m_impl->m_renderTarget) {
            D2D1_MATRIX_3X2_F current;
            m_impl->m_renderTarget->GetTransform(&current);
            m_impl->m_renderTarget->SetTransform(
                D2D1::Matrix3x2F::Scale(sx, sy, Helpers::TypeConversion::ToD2DPoint(center)) * current
            );
        }
    }

    void GraphicsCore::Translate(float dx, float dy) {
        if (m_impl->m_renderTarget) {
            D2D1_MATRIX_3X2_F current;
            m_impl->m_renderTarget->GetTransform(&current);
            m_impl->m_renderTarget->SetTransform(
                D2D1::Matrix3x2F::Translation(dx, dy) * current
            );
        }
    }

    void GraphicsCore::SetTransform(const D2D1_MATRIX_3X2_F& matrix) {
        if (m_impl->m_renderTarget) m_impl->m_renderTarget->SetTransform(matrix);
    }

    void GraphicsCore::ResetTransform() {
        if (m_impl->m_renderTarget) m_impl->m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    }

    ID2D1SolidColorBrush* GraphicsCore::GetSolidBrush(const Color& color) {
        VALIDATE_PTR_OR_RETURN_NULL(m_impl->m_renderTarget.Get(), "GraphicsCore");

        if (!m_impl->m_solidBrush) {
            m_impl->m_renderTarget->CreateSolidColorBrush(
                Helpers::TypeConversion::ToD2DColor(color),
                &m_impl->m_solidBrush
            );
        }
        else {
            m_impl->m_solidBrush->SetColor(Helpers::TypeConversion::ToD2DColor(color));
        }

        return m_impl->m_solidBrush.Get();
    }

    ID2D1LinearGradientBrush* GraphicsCore::GetLinearGradient(
        const Point& start, const Point& end, const std::vector<GradientStop>& stops) {

        const size_t hash = m_impl->HashGradientStops(stops);

        return m_impl->m_linearGradientCache.GetOrCreate(hash, [&]() {
            auto collection = Internal::CreateGradientStopCollection(m_impl->m_renderTarget.Get(), stops);
            if (!collection) return wrl::ComPtr<ID2D1LinearGradientBrush>();

            wrl::ComPtr<ID2D1LinearGradientBrush> brush;
            D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = D2D1::LinearGradientBrushProperties(
                Helpers::TypeConversion::ToD2DPoint(start),
                Helpers::TypeConversion::ToD2DPoint(end)
            );
            m_impl->m_renderTarget->CreateLinearGradientBrush(props, collection.Get(), &brush);
            return brush;
            }).Get();
    }

    ID2D1RadialGradientBrush* GraphicsCore::GetRadialGradient(
        const Point& center, float radiusX, float radiusY, const std::vector<GradientStop>& stops) {

        const size_t hash = m_impl->HashGradientStops(stops);

        return m_impl->m_radialGradientCache.GetOrCreate(hash, [&]() {
            auto collection = Internal::CreateGradientStopCollection(m_impl->m_renderTarget.Get(), stops);
            if (!collection) return wrl::ComPtr<ID2D1RadialGradientBrush>();

            wrl::ComPtr<ID2D1RadialGradientBrush> brush;
            D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props = D2D1::RadialGradientBrushProperties(
                Helpers::TypeConversion::ToD2DPoint(center),
                D2D1::Point2F(0.0f, 0.0f),
                radiusX, radiusY
            );
            m_impl->m_renderTarget->CreateRadialGradientBrush(props, collection.Get(), &brush);
            return brush;
            }).Get();
    }

    ID2D1Brush* GraphicsCore::GetBrushFromPaint(const Paint& paint, float globalAlpha) {
        VALIDATE_PTR_OR_RETURN_NULL(m_impl->m_renderTarget.Get(), "GraphicsCore");

        switch (paint.GetBrushType()) {
        case BrushType::Solid: {
            Color color = paint.GetColor();
            color.a *= globalAlpha * paint.GetAlpha();
            return GetSolidBrush(color);
        }
        case BrushType::LinearGradient:
            return GetLinearGradient(paint.GetLinearStart(), paint.GetLinearEnd(), paint.GetGradientStops());
        case BrushType::RadialGradient:
            return GetRadialGradient(paint.GetRadialCenter(), paint.GetRadialRadiusX(),
                paint.GetRadialRadiusY(), paint.GetGradientStops());
        default:
            return GetSolidBrush(Color(1, 1, 1, globalAlpha));
        }
    }

    wrl::ComPtr<ID2D1PathGeometry> GraphicsCore::CreatePathGeometry(std::function<void(ID2D1GeometrySink*)> buildFunc) {
        return Internal::CreatePathGeometry(m_impl->m_d2dFactory.Get(), buildFunc, "GraphicsCore");
    }

    void GraphicsCore::BeginOpacityLayer(float opacity) {
        VALIDATE_PTR_OR_RETURN(m_impl->m_renderTarget.Get(), "GraphicsCore");
        wrl::ComPtr<ID2D1Layer> layer;
        if (SUCCEEDED(m_impl->m_renderTarget->CreateLayer(nullptr, &layer))) {
            D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
            params.opacity = Helpers::Sanitize::NormalizedFloat(opacity);
            m_impl->m_renderTarget->PushLayer(&params, layer.Get());
        }
    }

    void GraphicsCore::EndOpacityLayer() {
        if (m_impl->m_renderTarget) m_impl->m_renderTarget->PopLayer();
    }

    void GraphicsCore::PushClipRect(const Rect& rect) {
        if (m_impl->m_renderTarget) {
            m_impl->m_renderTarget->PushAxisAlignedClip(
                D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height),
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
            );
        }
    }

    void GraphicsCore::PopClipRect() {
        if (m_impl->m_renderTarget) m_impl->m_renderTarget->PopAxisAlignedClip();
    }

    void GraphicsCore::ClearCache() {
        m_impl->m_solidBrush.Reset();
        m_impl->m_linearGradientCache.Clear();
        m_impl->m_radialGradientCache.Clear();
    }

    ID2D1Factory* GraphicsCore::GetFactory() const noexcept { return m_impl->m_d2dFactory.Get(); }
    IDWriteFactory* GraphicsCore::GetDWriteFactory() const noexcept { return m_impl->m_dwriteFactory.Get(); }
    ID2D1RenderTarget* GraphicsCore::GetRenderTarget() const noexcept { return m_impl->m_renderTarget.Get(); }
    ID3D11Device* GraphicsCore::GetD3D11Device() const noexcept { return m_impl->m_d3dDevice.Get(); }
    ID3D11DeviceContext* GraphicsCore::GetD3D11Context() const noexcept { return m_impl->m_d3dContext.Get(); }
    ID3D11RenderTargetView* GraphicsCore::GetD3D11RenderTargetView() const noexcept { return m_impl->m_renderTargetView.Get(); }
    IDXGISwapChain* GraphicsCore::GetSwapChain() const noexcept { return m_impl->m_swapChain.Get(); }
    int GraphicsCore::GetWidth() const noexcept { return m_impl->m_width; }
    int GraphicsCore::GetHeight() const noexcept { return m_impl->m_height; }

    bool GraphicsCore::Impl::InitializeFactories() {
        D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory), &options, reinterpret_cast<void**>(m_d2dFactory.GetAddressOf()))))
            return false;

        return SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())));
    }

    bool GraphicsCore::Impl::CreateRenderTarget() {
        if (m_windowMode == WindowMode::Overlay) {
            m_alphaDC = Helpers::Gdi::CreateAlphaDC(m_width, m_height);
            if (!m_alphaDC.IsValid()) return false;

            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );

            wrl::ComPtr<ID2D1DCRenderTarget> dcTarget;
            if (FAILED(m_d2dFactory->CreateDCRenderTarget(&props, &dcTarget)))
                return false;

            m_renderTarget = dcTarget;
        }
        else {
            D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );

            D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
                m_hwnd, D2D1::SizeU(m_width, m_height)
            );

            wrl::ComPtr<ID2D1HwndRenderTarget> hwndTarget;
            if (FAILED(m_d2dFactory->CreateHwndRenderTarget(rtProps, hwndProps, &hwndTarget)))
                return false;

            hwndTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            hwndTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            m_renderTarget = hwndTarget;
        }

        return true;
    }

    bool GraphicsCore::Impl::CreateD3D11Resources() {
        UINT createFlags = 0;
#ifdef _DEBUG
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        constexpr D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
        };

        D3D_FEATURE_LEVEL featureLevel;
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
            featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
            &m_d3dDevice, &featureLevel, &m_d3dContext)))
            return false;

        wrl::ComPtr<IDXGIDevice> dxgiDevice;
        if (FAILED(m_d3dDevice.As(&dxgiDevice))) return false;

        wrl::ComPtr<IDXGIAdapter> adapter;
        if (FAILED(dxgiDevice->GetAdapter(&adapter))) return false;

        wrl::ComPtr<IDXGIFactory> factory;
        if (FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) return false;

        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferDesc.Width = m_width;
        desc.BufferDesc.Height = m_height;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.RefreshRate = { 60, 1 };
        desc.SampleDesc = { 1, 0 };
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.OutputWindow = m_hwnd;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        if (FAILED(factory->CreateSwapChain(m_d3dDevice.Get(), &desc, &m_swapChain)))
            return false;

        wrl::ComPtr<ID3D11Texture2D> backBuffer;
        if (FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
            return false;

        if (FAILED(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView)))
            return false;

        m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

        D3D11_VIEWPORT vp = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
        m_d3dContext->RSSetViewports(1, &vp);

        return true;
    }

    void GraphicsCore::Impl::HandleDeviceLost() {
        m_solidBrush.Reset();
        m_linearGradientCache.Clear();
        m_radialGradientCache.Clear();
        CreateRenderTarget();
    }

    size_t GraphicsCore::Impl::HashGradientStops(const std::vector<GradientStop>& stops) const noexcept {
        size_t hash = 0;
        for (const auto& stop : stops) {
            Helpers::Rendering::HashGenerator::HashCombine(hash, stop.position);
            Helpers::Rendering::HashGenerator::HashCombine(hash, stop.color.r);
            Helpers::Rendering::HashGenerator::HashCombine(hash, stop.color.g);
            Helpers::Rendering::HashGenerator::HashCombine(hash, stop.color.b);
            Helpers::Rendering::HashGenerator::HashCombine(hash, stop.color.a);
        }
        return hash;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // GeometryBuilder Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GeometryBuilder::GeometryBuilder(ID2D1Factory* factory) : m_factory(factory) {}

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreatePathFromPoints(
        const std::vector<Point>& points, bool closed, bool filled) const {

        if (!Helpers::Sanitize::PointArray(points, 2)) return nullptr;

        return Internal::CreatePathGeometry(m_factory, [&](ID2D1GeometrySink* sink) {
            sink->BeginFigure(
                Helpers::TypeConversion::ToD2DPoint(points[0]),
                filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW
            );

            for (size_t i = 1; i < points.size(); ++i) {
                sink->AddLine(Helpers::TypeConversion::ToD2DPoint(points[i]));
            }

            sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
            }, "GeometryBuilder");
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateArc(
        const Point& center, float radius, float startAngle, float sweepAngle) const {

        if (!Helpers::Sanitize::PositiveRadius(radius) || !Helpers::Sanitize::NonZeroAngle(sweepAngle))
            return nullptr;

        return Internal::CreatePathGeometry(m_factory, [&](ID2D1GeometrySink* sink) {
            const float startRad = Helpers::Math::DegreesToRadians(startAngle);
            const float endRad = Helpers::Math::DegreesToRadians(startAngle + sweepAngle);

            const Point startPoint = Helpers::Geometry::PointOnCircle(center, radius, startRad);
            const Point endPoint = Helpers::Geometry::PointOnCircle(center, radius, endRad);

            sink->BeginFigure(Helpers::TypeConversion::ToD2DPoint(startPoint), D2D1_FIGURE_BEGIN_HOLLOW);

            D2D1_ARC_SEGMENT arc = {
                Helpers::TypeConversion::ToD2DPoint(endPoint),
                D2D1::SizeF(radius, radius),
                0.0f,
                sweepAngle > 0.0f ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                std::abs(sweepAngle) >= 180.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
            };

            sink->AddArc(&arc);
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            }, "GeometryBuilder");
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateRegularPolygon(
        const Point& center, float radius, int sides, float rotation) const {

        if (!Helpers::Sanitize::PositiveRadius(radius)) return nullptr;

        auto vertices = GenerateRegularPolygonVertices(center, radius, sides, rotation);
        return CreatePathFromPoints(vertices, true, true);
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateAngularSlice(
        const Point& center, float radius, float startAngle, float endAngle) const {

        if (!Helpers::Sanitize::PositiveRadius(radius)) return nullptr;

        return Internal::CreatePathGeometry(m_factory, [&](ID2D1GeometrySink* sink) {
            const Point startPoint = Helpers::Geometry::PointOnCircle(center, radius,
                Helpers::Math::DegreesToRadians(startAngle));
            const Point endPoint = Helpers::Geometry::PointOnCircle(center, radius,
                Helpers::Math::DegreesToRadians(endAngle));

            sink->BeginFigure(Helpers::TypeConversion::ToD2DPoint(center), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(Helpers::TypeConversion::ToD2DPoint(startPoint));

            D2D1_ARC_SEGMENT arc = {
                Helpers::TypeConversion::ToD2DPoint(endPoint),
                D2D1::SizeF(radius, radius),
                0.0f,
                (endAngle - startAngle) > 0.0f ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                std::abs(endAngle - startAngle) >= 180.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
            };

            sink->AddArc(&arc);
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            }, "GeometryBuilder");
    }

    std::vector<Point> GeometryBuilder::GenerateCirclePoints(const Point& center, float radius, int segments) {
        segments = Helpers::Sanitize::CircleSegments(segments);

        std::vector<Point> points;
        points.reserve(segments + 1);

        const float angleStep = TWO_PI / segments;
        for (int i = 0; i <= segments; ++i) {
            points.push_back(Helpers::Geometry::PointOnCircle(center, radius, i * angleStep));
        }

        return points;
    }

    std::vector<Point> GeometryBuilder::GenerateStarVertices(
        const Point& center, float outerRadius, float innerRadius, int points) {

        points = Helpers::Sanitize::StarPoints(points);

        std::vector<Point> vertices;
        vertices.reserve(points * 2);

        const float angleStep = PI / points;
        float angle = -PI / 2.0f;

        for (int i = 0; i < points * 2; ++i) {
            float radius = (i & 1) ? innerRadius : outerRadius;
            vertices.push_back(Helpers::Geometry::PointOnCircle(center, radius, angle));
            angle += angleStep;
        }

        return vertices;
    }

    std::vector<Point> GeometryBuilder::GenerateRegularPolygonVertices(
        const Point& center, float radius, int sides, float rotation) {

        sides = Helpers::Sanitize::PolygonSides(sides);

        std::vector<Point> vertices;
        vertices.reserve(sides);

        const float angleStep = TWO_PI / sides;
        const float startAngle = Helpers::Math::DegreesToRadians(rotation);

        for (int i = 0; i < sides; ++i) {
            vertices.push_back(Helpers::Geometry::PointOnCircle(center, radius, startAngle + i * angleStep));
        }

        return vertices;
    }

    std::vector<Point> GeometryBuilder::GenerateWaveformPoints(
        const SpectrumData& spectrum, const Rect& bounds) {

        if (spectrum.size() < 2) return {};

        std::vector<Point> points;
        points.reserve(spectrum.size());

        const float midY = bounds.y + bounds.height * 0.5f;
        const float amplitude = bounds.height * 0.5f;
        const float stepX = bounds.width / (spectrum.size() - 1);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            points.push_back({
                bounds.x + i * stepX,
                midY - Helpers::Sanitize::NormalizedFloat(spectrum[i]) * amplitude
                });
        }

        return points;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RenderEngine Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct RenderEngine::Impl {
        GraphicsCore m_core;
        std::unique_ptr<Renderer> m_renderer;
        std::unique_ptr<Canvas> m_canvas;
        HWND m_hwnd;
        WindowMode m_windowMode;
        RenderMode m_renderMode;

        Impl(HWND hwnd, WindowMode windowMode, RenderMode renderMode)
            : m_hwnd(hwnd), m_windowMode(windowMode), m_renderMode(renderMode) {
        }

        void CreateComponents() {
            auto factory = m_core.GetFactory();
            auto dwriteFactory = m_core.GetDWriteFactory();

            if (factory && dwriteFactory) {
                m_renderer = std::make_unique<Renderer>(factory, dwriteFactory);
                m_renderer->SetRenderTarget(m_core.GetRenderTarget());
                m_canvas = std::make_unique<Canvas>(m_renderer.get(), &m_core);
            }
        }
    };

    RenderEngine::DrawScope::DrawScope(RenderEngine& engine)
        : m_engine(engine), m_begun(engine.BeginDraw()) {
    }

    RenderEngine::DrawScope::~DrawScope() noexcept {
        if (m_begun) {
            try { (void)m_engine.EndDraw(); }
            catch (...) {}
        }
    }

    bool RenderEngine::DrawScope::IsActive() const noexcept { return m_begun; }

    RenderEngine::RenderEngine(HWND hwnd, WindowMode windowMode, RenderMode renderMode)
        : m_impl(std::make_unique<Impl>(hwnd, windowMode, renderMode)) {
    }

    RenderEngine::~RenderEngine() noexcept = default;

    bool RenderEngine::Initialize() {
        VALIDATE_CONDITION_OR_RETURN_FALSE(
            m_impl->m_hwnd && ::IsWindow(m_impl->m_hwnd),
            "Invalid HWND", "RenderEngine"
        );

        const bool success = (m_impl->m_renderMode == RenderMode::Direct2D)
            ? m_impl->m_core.InitializeD2D(m_impl->m_hwnd, m_impl->m_windowMode)
            : m_impl->m_core.InitializeD3D11(m_impl->m_hwnd);

        if (success && m_impl->m_renderMode == RenderMode::Direct2D) {
            m_impl->CreateComponents();
        }

        return success;
    }

    void RenderEngine::Resize(int width, int height) {
        if (m_impl->m_core.IsDrawing()) return;

        m_impl->m_core.RecreateResources(width, height);

        if (m_impl->m_renderer) {
            m_impl->m_renderer->SetRenderTarget(m_impl->m_core.GetRenderTarget());
        }
    }

    bool RenderEngine::BeginDraw() {
        return m_impl->m_renderMode == RenderMode::Direct2D ? m_impl->m_core.BeginDraw() : false;
    }

    HRESULT RenderEngine::EndDraw() {
        return m_impl->m_renderMode == RenderMode::Direct2D ? m_impl->m_core.EndDraw() : E_FAIL;
    }

    RenderEngine::DrawScope RenderEngine::CreateDrawScope() { return DrawScope(*this); }

    void RenderEngine::Clear(const Color& color) {
        if (m_impl->m_renderMode == RenderMode::Direct2D) {
            m_impl->m_core.Clear(color);
        }
    }

    void RenderEngine::ClearD3D11(const Color& color) {
        if (m_impl->m_renderMode != RenderMode::Direct3D11) return;

        auto context = m_impl->m_core.GetD3D11Context();
        auto rtv = m_impl->m_core.GetD3D11RenderTargetView();

        if (context && rtv) {
            const float clearColor[4] = { color.r, color.g, color.b, color.a };
            context->ClearRenderTargetView(rtv, clearColor);
        }
    }

    void RenderEngine::Present() {
        if (m_impl->m_renderMode == RenderMode::Direct3D11) {
            if (auto swapChain = m_impl->m_core.GetSwapChain()) {
                swapChain->Present(1, 0);
            }
        }
    }

    Canvas& RenderEngine::GetCanvas() {
        if (!m_impl->m_canvas) {
            static Canvas dummy(nullptr, nullptr);
            return dummy;
        }
        return *m_impl->m_canvas;
    }

    const Canvas& RenderEngine::GetCanvas() const {
        return const_cast<RenderEngine*>(this)->GetCanvas();
    }

    GraphicsCore& RenderEngine::GetCore() noexcept { return m_impl->m_core; }
    const GraphicsCore& RenderEngine::GetCore() const noexcept { return m_impl->m_core; }
    ID3D11Device* RenderEngine::GetD3D11Device() const noexcept { return m_impl->m_core.GetD3D11Device(); }
    ID3D11DeviceContext* RenderEngine::GetD3D11DeviceContext() const noexcept { return m_impl->m_core.GetD3D11Context(); }
    ID3D11RenderTargetView* RenderEngine::GetD3D11RenderTargetView() const noexcept { return m_impl->m_core.GetD3D11RenderTargetView(); }
    int RenderEngine::GetWidth() const noexcept { return m_impl->m_core.GetWidth(); }
    int RenderEngine::GetHeight() const noexcept { return m_impl->m_core.GetHeight(); }
    bool RenderEngine::IsDrawing() const noexcept { return m_impl->m_core.IsDrawing(); }
    RenderMode RenderEngine::GetRenderMode() const noexcept { return m_impl->m_renderMode; }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Renderer Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct Renderer::Impl {
        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;
        wrl::ComPtr<ID2D1Factory> m_d2dFactory;
        wrl::ComPtr<IDWriteFactory> m_writeFactory;
        Helpers::Rendering::RenderResourceCache<uint32_t, ID2D1SolidColorBrush> m_brushCache;
        Helpers::Rendering::RenderResourceCache<size_t, IDWriteTextFormat> m_formatCache;

        ID2D1Brush* GetBrush(const Paint& paint);
        ID2D1SolidColorBrush* GetSolidBrush(const Color& color);
        IDWriteTextFormat* GetTextFormat(const TextStyle& style);
        void DrawShape(const std::function<void(ID2D1RenderTarget*, ID2D1Brush*)>& drawFunc, const Paint& paint);
    };

    Renderer::Renderer(ID2D1Factory* d2dFactory, IDWriteFactory* writeFactory)
        : m_impl(std::make_unique<Impl>()) {
        m_impl->m_d2dFactory = d2dFactory;
        m_impl->m_writeFactory = writeFactory;
    }

    Renderer::~Renderer() noexcept = default;

    void Renderer::SetRenderTarget(ID2D1RenderTarget* renderTarget) {
        if (m_impl->m_renderTarget.Get() != renderTarget) {
            m_impl->m_brushCache.Clear();
            m_impl->m_formatCache.Clear();
            m_impl->m_renderTarget = renderTarget;
        }
    }

    void Renderer::OnDeviceLost() {
        m_impl->m_brushCache.Clear();
        m_impl->m_formatCache.Clear();
        m_impl->m_renderTarget.Reset();
    }

    void Renderer::DrawText(const std::wstring& text, const Rect& rect, const TextStyle& style) {
        if (!Helpers::Rendering::RenderValidation::ValidateTextRenderingContext(
            m_impl->m_renderTarget.Get(), m_impl->m_writeFactory.Get(), text)) return;

        auto format = m_impl->GetTextFormat(style);
        auto brush = m_impl->GetSolidBrush(style.color);

        if (format && brush) {
            m_impl->m_renderTarget->DrawTextW(
                text.c_str(),
                static_cast<UINT32>(text.length()),
                format,
                Helpers::TypeConversion::ToD2DRect(rect),
                brush
            );
        }
    }

    void Renderer::DrawRectangle(const Rect& rect, const Paint& paint) {
        m_impl->DrawShape([&](ID2D1RenderTarget* rt, ID2D1Brush* brush) {
            const auto d2dRect = Helpers::TypeConversion::ToD2DRect(rect);
            if (paint.IsFilled()) rt->FillRectangle(&d2dRect, brush);
            if (paint.IsStroked()) rt->DrawRectangle(&d2dRect, brush, paint.GetStrokeWidth());
            }, paint);
    }

    void Renderer::DrawRoundedRectangle(const Rect& rect, float radius, const Paint& paint) {
        m_impl->DrawShape([&](ID2D1RenderTarget* rt, ID2D1Brush* brush) {
            const D2D1_ROUNDED_RECT rr = { Helpers::TypeConversion::ToD2DRect(rect), radius, radius };
            if (paint.IsFilled()) rt->FillRoundedRectangle(&rr, brush);
            if (paint.IsStroked()) rt->DrawRoundedRectangle(&rr, brush, paint.GetStrokeWidth());
            }, paint);
    }

    void Renderer::DrawEllipse(const Point& center, float radiusX, float radiusY, const Paint& paint) {
        m_impl->DrawShape([&](ID2D1RenderTarget* rt, ID2D1Brush* brush) {
            const D2D1_ELLIPSE ellipse = Helpers::TypeConversion::ToD2DEllipse(center, radiusX, radiusY);
            if (paint.IsFilled()) rt->FillEllipse(&ellipse, brush);
            if (paint.IsStroked()) rt->DrawEllipse(&ellipse, brush, paint.GetStrokeWidth());
            }, paint);
    }

    void Renderer::DrawLine(const Point& start, const Point& end, const Paint& paint) {
        m_impl->DrawShape([&](ID2D1RenderTarget* rt, ID2D1Brush* brush) {
            rt->DrawLine(
                Helpers::TypeConversion::ToD2DPoint(start),
                Helpers::TypeConversion::ToD2DPoint(end),
                brush, paint.GetStrokeWidth()
            );
            }, paint);
    }

    void Renderer::DrawGeometry(ID2D1Geometry* geometry, const Paint& paint) {
        if (!geometry) return;
        m_impl->DrawShape([&](ID2D1RenderTarget* rt, ID2D1Brush* brush) {
            rt->DrawGeometry(geometry, brush, paint.GetStrokeWidth());
            }, paint);
    }

    void Renderer::FillGeometry(ID2D1Geometry* geometry, const Paint& paint) {
        if (!geometry) return;
        m_impl->DrawShape([&](ID2D1RenderTarget* rt, ID2D1Brush* brush) {
            rt->FillGeometry(geometry, brush);
            }, paint);
    }

    ID2D1Brush* Renderer::GetBrush(const Paint& paint) { return m_impl->GetBrush(paint); }
    ID2D1SolidColorBrush* Renderer::GetSolidBrush(const Color& color) { return m_impl->GetSolidBrush(color); }

    wrl::ComPtr<ID2D1PathGeometry> Renderer::CreatePath(const std::vector<Point>& points, bool closed) {
        if (!Helpers::Rendering::RenderValidation::ValidatePointArray(points, closed ? 3 : 2))
            return nullptr;

        return Internal::CreatePathGeometry(m_impl->m_d2dFactory.Get(), [&](ID2D1GeometrySink* sink) {
            sink->BeginFigure(Helpers::TypeConversion::ToD2DPoint(points[0]), D2D1_FIGURE_BEGIN_FILLED);

            for (size_t i = 1; i < points.size(); ++i) {
                sink->AddLine(Helpers::TypeConversion::ToD2DPoint(points[i]));
            }

            sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
            }, "Renderer");
    }

    wrl::ComPtr<ID2D1PathGeometry> Renderer::CreatePathFromLines(const std::vector<Point>& points) {
        return CreatePath(points, false);
    }

    ID2D1Factory* Renderer::GetFactory() const noexcept { return m_impl->m_d2dFactory.Get(); }
    ID2D1RenderTarget* Renderer::GetRenderTarget() const noexcept { return m_impl->m_renderTarget.Get(); }
    IDWriteFactory* Renderer::GetWriteFactory() const noexcept { return m_impl->m_writeFactory.Get(); }

    ID2D1Brush* Renderer::Impl::GetBrush(const Paint& paint) {
        if (!m_renderTarget) return nullptr;

        switch (paint.GetBrushType()) {
        case BrushType::Solid:
            return GetSolidBrush(paint.GetColor().WithAlpha(paint.GetAlpha()));

        case BrushType::LinearGradient: {
            auto collection = Internal::CreateGradientStopCollection(
                m_renderTarget.Get(), paint.GetGradientStops(), paint.GetAlpha()
            );
            if (!collection) return nullptr;

            wrl::ComPtr<ID2D1LinearGradientBrush> brush;
            D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = D2D1::LinearGradientBrushProperties(
                Helpers::TypeConversion::ToD2DPoint(paint.GetLinearStart()),
                Helpers::TypeConversion::ToD2DPoint(paint.GetLinearEnd())
            );
            m_renderTarget->CreateLinearGradientBrush(props, collection.Get(), &brush);
            return brush.Detach();
        }

        case BrushType::RadialGradient: {
            auto collection = Internal::CreateGradientStopCollection(
                m_renderTarget.Get(), paint.GetGradientStops(), paint.GetAlpha()
            );
            if (!collection) return nullptr;

            wrl::ComPtr<ID2D1RadialGradientBrush> brush;
            D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props = D2D1::RadialGradientBrushProperties(
                Helpers::TypeConversion::ToD2DPoint(paint.GetRadialCenter()),
                D2D1::Point2F(0.0f, 0.0f),
                paint.GetRadialRadiusX(),
                paint.GetRadialRadiusY()
            );
            m_renderTarget->CreateRadialGradientBrush(props, collection.Get(), &brush);
            return brush.Detach();
        }
        }

        return nullptr;
    }

    ID2D1SolidColorBrush* Renderer::Impl::GetSolidBrush(const Color& color) {
        if (!m_renderTarget) return nullptr;

        const uint32_t key = Helpers::ColorHelpers::ColorToARGB(color);

        return m_brushCache.GetOrCreate(key, [&]() {
            return Helpers::Rendering::BrushManager::CreateSolidBrush(m_renderTarget.Get(), color);
            }).Get();
    }

    IDWriteTextFormat* Renderer::Impl::GetTextFormat(const TextStyle& style) {
        if (!m_writeFactory) return nullptr;

        const size_t key = Helpers::Rendering::HashGenerator::GenerateTextFormatKey(
            style.fontFamily, style.fontSize,
            Helpers::EnumConversion::ToDWriteFontWeight(style.weight),
            Helpers::EnumConversion::ToDWriteFontStyle(style.style),
            Helpers::EnumConversion::ToDWriteFontStretch(style.stretch),
            Helpers::EnumConversion::ToDWriteTextAlign(style.textAlign),
            Helpers::EnumConversion::ToDWriteParagraphAlign(style.paragraphAlign)
        );

        return m_formatCache.GetOrCreate(key, [&]() {
            auto format = Helpers::Rendering::FactoryHelper::CreateTextFormat(
                m_writeFactory.Get(), style.fontFamily, style.fontSize,
                Helpers::EnumConversion::ToDWriteFontWeight(style.weight),
                Helpers::EnumConversion::ToDWriteFontStyle(style.style),
                Helpers::EnumConversion::ToDWriteFontStretch(style.stretch)
            );

            if (format) {
                format->SetTextAlignment(Helpers::EnumConversion::ToDWriteTextAlign(style.textAlign));
                format->SetParagraphAlignment(Helpers::EnumConversion::ToDWriteParagraphAlign(style.paragraphAlign));
            }

            return format;
            }).Get();
    }

    void Renderer::Impl::DrawShape(
        const std::function<void(ID2D1RenderTarget*, ID2D1Brush*)>& drawFunc,
        const Paint& paint) {

        if (!m_renderTarget) return;

        wrl::ComPtr<ID2D1Brush> brush;
        if (paint.GetBrushType() == BrushType::Solid) {
            brush = GetBrush(paint);
        }
        else {
            brush.Attach(GetBrush(paint));
        }

        if (brush && Helpers::Rendering::RenderValidation::ValidateBrush(brush.Get())) {
            drawFunc(m_renderTarget.Get(), brush.Get());
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Canvas Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Canvas::Canvas(Renderer* renderer, GraphicsCore* core)
        : m_renderer(renderer), m_core(core) {
    }

    Canvas::~Canvas() noexcept = default;

    ID2D1RenderTarget* Canvas::GetRenderTarget() const noexcept {
        return m_core ? m_core->GetRenderTarget() : nullptr;
    }

    void Canvas::DrawRectangle(const Rect& rect, const Paint& paint) const {
        if (m_renderer) m_renderer->DrawRectangle(rect, paint);
    }

    void Canvas::DrawRoundedRectangle(const Rect& rect, float radius, const Paint& paint) const {
        if (m_renderer) m_renderer->DrawRoundedRectangle(rect, radius, paint);
    }

    void Canvas::DrawCircle(const Point& center, float radius, const Paint& paint) const {
        if (m_renderer) m_renderer->DrawEllipse(center, radius, radius, paint);
    }

    void Canvas::DrawEllipse(const Point& center, float radiusX, float radiusY, const Paint& paint) const {
        if (m_renderer) m_renderer->DrawEllipse(center, radiusX, radiusY, paint);
    }

    void Canvas::DrawLine(const Point& start, const Point& end, const Paint& paint) const {
        if (m_renderer) m_renderer->DrawLine(start, end, paint);
    }

    void Canvas::DrawPolyline(const std::vector<Point>& points, const Paint& paint) const {
        if (!m_renderer || points.size() < 2) return;

        if (auto path = m_renderer->CreatePathFromLines(points)) {
            m_renderer->DrawGeometry(path.Get(), paint);
        }
    }

    void Canvas::DrawPolygon(const std::vector<Point>& points, const Paint& paint) const {
        if (!m_renderer || points.size() < 3) return;

        if (auto path = m_renderer->CreatePath(points, true)) {
            if (paint.IsFilled()) m_renderer->FillGeometry(path.Get(), paint);
            if (paint.IsStroked()) m_renderer->DrawGeometry(path.Get(), paint);
        }
    }

    void Canvas::DrawArc(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const {
        using namespace Constants::Geometry;

        const int segments = Helpers::Math::Clamp(
            static_cast<int>(std::abs(sweepAngle) / kDegreesPerSegment),
            kMinCircleSegments, kMaxCircleSegments
        );

        std::vector<Point> points;
        points.reserve(segments + 1);

        const float angleStep = Helpers::Math::DegreesToRadians(sweepAngle) / segments;
        const float startRad = Helpers::Math::DegreesToRadians(startAngle);

        for (int i = 0; i <= segments; ++i) {
            points.push_back(Helpers::Geometry::PointOnCircle(center, radius, startRad + i * angleStep));
        }

        DrawPolyline(points, paint);
    }

    void Canvas::DrawRing(const Point& center, float innerRadius, float outerRadius, const Paint& paint) const {
        if (!m_renderer || innerRadius >= outerRadius || innerRadius < 0) return;

        const float midRadius = (innerRadius + outerRadius) * 0.5f;
        const float strokeWidth = outerRadius - innerRadius;

        DrawCircle(center, midRadius, Paint::Stroke(paint.GetColor(), strokeWidth).WithAlpha(paint.GetAlpha()));
    }

    void Canvas::DrawSector(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const {
        using namespace Constants::Geometry;

        const int segments = Helpers::Math::Clamp(
            static_cast<int>(std::abs(sweepAngle) / kDegreesPerSegment),
            kMinCircleSegments, kMaxCircleSegments
        );

        std::vector<Point> points;
        points.reserve(segments + 2);
        points.push_back(center);

        const float angleStep = Helpers::Math::DegreesToRadians(sweepAngle) / segments;
        const float startRad = Helpers::Math::DegreesToRadians(startAngle);

        for (int i = 0; i <= segments; ++i) {
            points.push_back(Helpers::Geometry::PointOnCircle(center, radius, startRad + i * angleStep));
        }

        DrawPolygon(points, paint);
    }

    void Canvas::DrawRegularPolygon(const Point& center, float radius, int sides, float rotation, const Paint& paint) const {
        DrawPolygon(GeometryBuilder::GenerateRegularPolygonVertices(center, radius, sides, rotation), paint);
    }

    void Canvas::DrawStar(const Point& center, float outerRadius, float innerRadius, int points, const Paint& paint) const {
        DrawPolygon(GeometryBuilder::GenerateStarVertices(center, outerRadius, innerRadius, points), paint);
    }

    void Canvas::DrawGrid(const Rect& bounds, int rows, int cols, const Paint& paint) const {
        if (rows <= 0 || cols <= 0) return;

        const float dx = bounds.width / cols;
        const float dy = bounds.height / rows;

        for (int i = 1; i < cols; ++i) {
            const float x = bounds.x + i * dx;
            DrawLine({ x, bounds.y }, { x, bounds.y + bounds.height }, paint);
        }

        for (int i = 1; i < rows; ++i) {
            const float y = bounds.y + i * dy;
            DrawLine({ bounds.x, y }, { bounds.x + bounds.width, y }, paint);
        }
    }

    void Canvas::DrawGlow(const Point& center, float radius, const Color& glowColor, float intensity, int layers) const {
        Internal::DrawGlowEffect(
            [&](float expansion, const Color& layerColor) {
                DrawCircle(center, radius + expansion, Paint::Fill(layerColor));
            },
            glowColor, intensity, layers
        );
    }

    void Canvas::DrawRectangleGlow(const Rect& rect, const Color& glowColor, float intensity, int layers) const {
        Internal::DrawGlowEffect(
            [&](float expansion, const Color& layerColor) {
                Rect expandedRect{
                    rect.x - expansion, rect.y - expansion,
                    rect.width + 2 * expansion, rect.height + 2 * expansion
                };
                DrawRectangle(expandedRect, Paint::Stroke(layerColor, 1.0f));
            },
            glowColor, intensity, layers
        );
    }

    void Canvas::DrawRoundedRectangleGlow(const Rect& rect, float cornerRadius, const Color& glowColor, float intensity, int layers) const {
        Internal::DrawGlowEffect(
            [&](float expansion, const Color& layerColor) {
                Rect expandedRect{
                    rect.x - expansion, rect.y - expansion,
                    rect.width + 2 * expansion, rect.height + 2 * expansion
                };
                DrawRoundedRectangle(expandedRect, cornerRadius + expansion, Paint::Stroke(layerColor, 1.0f));
            },
            glowColor, intensity, layers
        );
    }

    void Canvas::DrawWithShadow(std::function<void()> drawCallback, const Point& offset, float blur, const Color& shadowColor) const {
        if (!drawCallback || !m_core) return;
        (void)blur; // blur parameter is unused

        PushTransform();
        TranslateBy(offset.x, offset.y);
        BeginOpacityLayer(shadowColor.a);
        drawCallback();
        EndOpacityLayer();
        PopTransform();

        drawCallback();
    }

    void Canvas::DrawCircleBatch(const std::vector<Point>& centers, float radius, const Paint& paint) const {
        for (const auto& center : centers) {
            DrawCircle(center, radius, paint);
        }
    }

    void Canvas::DrawRectangleBatch(const std::vector<Rect>& rects, const Paint& paint) const {
        for (const auto& rect : rects) {
            DrawRectangle(rect, paint);
        }
    }

    void Canvas::BeginOpacityLayer(float opacity) const {
        if (m_core) m_core->BeginOpacityLayer(opacity);
    }

    void Canvas::EndOpacityLayer() const {
        if (m_core) m_core->EndOpacityLayer();
    }

    void Canvas::PushClipRect(const Rect& rect) const {
        if (m_core) m_core->PushClipRect(rect);
    }

    void Canvas::PopClipRect() const {
        if (m_core) m_core->PopClipRect();
    }

    void Canvas::PushTransform() const {
        if (m_core) m_core->PushTransform();
    }

    void Canvas::PopTransform() const {
        if (m_core) m_core->PopTransform();
    }

    void Canvas::RotateAt(const Point& center, float angleDegrees) const {
        if (m_core) m_core->Rotate(center, angleDegrees);
    }

    void Canvas::ScaleAt(const Point& center, float scaleX, float scaleY) const {
        if (m_core) m_core->Scale(center, scaleX, scaleY);
    }

    void Canvas::TranslateBy(float dx, float dy) const {
        if (m_core) m_core->Translate(dx, dy);
    }

    void Canvas::SetTransform(const D2D1_MATRIX_3X2_F& transform) const {
        if (m_core) m_core->SetTransform(transform);
    }

    void Canvas::ResetTransform() const {
        if (m_core) m_core->ResetTransform();
    }

    void Canvas::DrawText(const std::wstring& text, const Rect& layoutRect, const TextStyle& style) const {
        if (m_renderer) m_renderer->DrawText(text, layoutRect, style);
    }

    void Canvas::DrawText(const std::wstring& text, const Point& position, const TextStyle& style) const {
        DrawText(text, { position.x, position.y, 1000.0f, 100.0f }, style);
    }

    void Canvas::DrawSpectrumBars(const SpectrumData& spectrum, const Rect& bounds, const BarStyle& style, const Color& color) const {
        if (!m_renderer || spectrum.empty()) return;

        using namespace Constants::Rendering;

        const float totalSpacing = style.spacing * (spectrum.size() + 1);
        const float availableWidth = bounds.width - totalSpacing;
        const float barWidth = availableWidth / spectrum.size();

        if (barWidth <= 0) return;

        const Paint paint = Paint::Fill(color);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float height = std::max(spectrum[i] * bounds.height, kMinBarHeight);
            const float x = bounds.x + style.spacing + i * (barWidth + style.spacing);
            const float y = bounds.y + bounds.height - height;

            DrawRoundedRectangle({ x, y, barWidth, height }, style.cornerRadius, paint);
        }
    }

    void Canvas::DrawWaveform(const SpectrumData& spectrum, const Rect& bounds, const Paint& paint, bool mirror) const {
        if (spectrum.size() < 2) return;

        using namespace Constants::Rendering;

        std::vector<Point> topPoints;
        topPoints.reserve(spectrum.size());

        const float dx = bounds.width / (spectrum.size() - 1);
        const float midY = bounds.y + bounds.height * 0.5f;
        const float amplitude = bounds.height * 0.5f;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            topPoints.push_back({
                bounds.x + i * dx,
                midY - Helpers::Sanitize::NormalizedFloat(spectrum[i]) * amplitude
                });
        }

        DrawPolyline(topPoints, paint);

        if (mirror) {
            std::vector<Point> bottomPoints;
            bottomPoints.reserve(spectrum.size());

            for (size_t i = 0; i < spectrum.size(); ++i) {
                bottomPoints.push_back({
                    bounds.x + i * dx,
                    midY + Helpers::Sanitize::NormalizedFloat(spectrum[i]) * amplitude
                    });
            }

            Paint mirrorPaint = paint;
            mirrorPaint.WithAlpha(paint.GetAlpha() * kMirrorAlphaFactor);
            DrawPolyline(bottomPoints, mirrorPaint);
        }
    }

} // namespace Spectrum