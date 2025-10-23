#ifndef SPECTRUM_GRAPHICS_API_H
#define SPECTRUM_GRAPHICS_API_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Unified Graphics API Header - REFACTORED
// Eliminates duplication, improves architecture, maintains single-file design
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/SpectrumTypes.h"
#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <dxgi.h>
#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 1: ENUMS & MODES
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class PaintStyle : uint8_t {
        Fill = 0,
        Stroke = 1,
        FillAndStroke = 2
    };

    enum class BrushType : uint8_t {
        Solid = 0,
        LinearGradient = 1,
        RadialGradient = 2
    };

    enum class StrokeCap : uint8_t {
        Flat = 0,
        Round = 1,
        Square = 2
    };

    enum class StrokeJoin : uint8_t {
        Miter = 0,
        Round = 1,
        Bevel = 2
    };

    enum class DashStyle : uint8_t {
        Solid = 0,
        Dash = 1,
        Dot = 2,
        DashDot = 3,
        DashDotDot = 4,
        Custom = 5
    };

    enum class AntiAliasMode : uint8_t {
        None = 0,
        PerPrimitive = 1
    };

    enum class FillRule : uint8_t {
        EvenOdd = 0,
        Winding = 1
    };

    enum class BlendMode : uint8_t {
        SourceOver = 0,
        SourceIn = 1,
        SourceOut = 2,
        DestinationOver = 3,
        DestinationIn = 4,
        DestinationOut = 5,
        Add = 6,
        Multiply = 7,
        Screen = 8,
        Overlay = 9,
        Xor = 10
    };

    enum class FilterQuality : uint8_t {
        None = 0,
        Low = 1,
        Medium = 2,
        High = 3
    };

    enum class TextAlign : uint8_t {
        Leading = 0,
        Trailing = 1,
        Center = 2,
        Justified = 3
    };

    enum class ParagraphAlign : uint8_t {
        Near = 0,
        Far = 1,
        Center = 2
    };

    enum class FontWeight : uint16_t {
        Thin = 100,
        ExtraLight = 200,
        Light = 300,
        Regular = 400,
        Medium = 500,
        SemiBold = 600,
        Bold = 700,
        ExtraBold = 800,
        Black = 900,
        Normal = Regular,
        Heavy = Black
    };

    enum class FontStyle : uint8_t {
        Normal = 0,
        Italic = 1,
        Oblique = 2
    };

    enum class FontStretch : uint8_t {
        UltraCondensed = 1,
        ExtraCondensed = 2,
        Condensed = 3,
        SemiCondensed = 4,
        Normal = 5,
        SemiExpanded = 6,
        Expanded = 7,
        ExtraExpanded = 8,
        UltraExpanded = 9
    };

    enum class TextDecoration : uint8_t {
        None = 0,
        Underline = 1 << 0,
        Strikethrough = 1 << 1,
        Overline = 1 << 2
    };

    enum class RenderMode : uint8_t {
        Direct2D = 0,
        Direct3D11 = 1
    };

    enum class WindowMode : uint8_t {
        Normal = 0,
        Overlay = 1
    };

    inline TextDecoration operator|(TextDecoration a, TextDecoration b) {
        return static_cast<TextDecoration>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline bool operator&(TextDecoration a, TextDecoration b) {
        return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 2: CONSTANTS
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Constants {
        namespace Geometry {
            constexpr int kMinCircleSegments = 8;
            constexpr int kMaxCircleSegments = 360;
            constexpr int kDefaultCircleSegments = 64;
            constexpr float kDegreesPerSegment = 5.0f;
            constexpr int kMinPolygonSides = 3;
            constexpr int kMaxPolygonSides = 360;
            constexpr int kMinStarPoints = 3;
            constexpr int kMaxStarPoints = 50;
            constexpr float PI = 3.14159265359f;
            constexpr float TWO_PI = 6.28318530718f;
        }

        namespace Effects {
            constexpr int kDefaultGlowLayers = 5;
            constexpr int kMinGlowLayers = 1;
            constexpr int kMaxGlowLayers = 10;
            constexpr float kGlowIntensityFactor = 0.2f;
            constexpr float kGlowExpansionStep = 2.0f;
        }

        namespace Rendering {
            constexpr float kMinBarHeight = 1.0f;
            constexpr float kMirrorAlphaFactor = 0.6f;
            constexpr int kMinSize = 1;
            constexpr int kMaxSize = 16384;
        }

        namespace Cache {
            constexpr size_t kMaxGradientBrushes = 1000;
            constexpr size_t kMaxTextFormats = 100;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 3: STRUCTS
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct GradientStop {
        float position;
        Color color;

        GradientStop() = default;
        GradientStop(float pos, const Color& col) : position(pos), color(col) {}
    };

    struct StrokeOptions {
        float width = 1.0f;
        StrokeCap cap = StrokeCap::Flat;
        StrokeJoin join = StrokeJoin::Miter;
        float miterLimit = 10.0f;
        DashStyle dashStyle = DashStyle::Solid;
        std::vector<float> dashPattern;
        float dashOffset = 0.0f;

        StrokeOptions& SetWidth(float w) { width = w; return *this; }
        StrokeOptions& SetCap(StrokeCap c) { cap = c; return *this; }
        StrokeOptions& SetJoin(StrokeJoin j) { join = j; return *this; }
        StrokeOptions& SetMiterLimit(float limit) { miterLimit = limit; return *this; }
        StrokeOptions& SetDashStyle(DashStyle style) { dashStyle = style; return *this; }
        StrokeOptions& SetDashPattern(const std::vector<float>& pattern) {
            dashPattern = pattern;
            dashStyle = DashStyle::Custom;
            return *this;
        }
        StrokeOptions& SetDashOffset(float offset) { dashOffset = offset; return *this; }

        [[nodiscard]] static StrokeOptions Default() { return {}; }
        [[nodiscard]] static StrokeOptions Round(float width) {
            return StrokeOptions{}.SetWidth(width).SetCap(StrokeCap::Round).SetJoin(StrokeJoin::Round);
        }
        [[nodiscard]] static StrokeOptions Dashed(float width, DashStyle style = DashStyle::Dash) {
            return StrokeOptions{}.SetWidth(width).SetDashStyle(style);
        }
    };

    struct TextStyle {
        std::wstring fontFamily = L"Segoe UI";
        float fontSize = 14.0f;
        FontWeight weight = FontWeight::Normal;
        FontStyle style = FontStyle::Normal;
        FontStretch stretch = FontStretch::Normal;
        TextAlign textAlign = TextAlign::Leading;
        ParagraphAlign paragraphAlign = ParagraphAlign::Near;
        float lineHeight = 1.2f;
        float letterSpacing = 0.0f;
        float wordSpacing = 0.0f;
        Color color = Color::White();
        Color outlineColor = Color::Transparent();
        float outlineWidth = 0.0f;
        TextDecoration decoration = TextDecoration::None;
        bool kerning = true;
        bool ligatures = true;
        float baseline = 0.0f;

        [[nodiscard]] TextStyle WithFont(const std::wstring& family) const {
            TextStyle result = *this; result.fontFamily = family; return result;
        }
        [[nodiscard]] TextStyle WithSize(float size) const {
            TextStyle result = *this; result.fontSize = std::max(1.0f, size); return result;
        }
        [[nodiscard]] TextStyle WithWeight(FontWeight w) const {
            TextStyle result = *this; result.weight = w; return result;
        }
        [[nodiscard]] TextStyle WithStyle(FontStyle s) const {
            TextStyle result = *this; result.style = s; return result;
        }
        [[nodiscard]] TextStyle WithColor(const Color& c) const {
            TextStyle result = *this; result.color = c; return result;
        }
        [[nodiscard]] TextStyle WithAlign(TextAlign align) const {
            TextStyle result = *this; result.textAlign = align; return result;
        }
        [[nodiscard]] TextStyle WithParagraphAlign(ParagraphAlign align) const {
            TextStyle result = *this; result.paragraphAlign = align; return result;
        }

        [[nodiscard]] static TextStyle Default() { return {}; }
        [[nodiscard]] static TextStyle Title() { return TextStyle{}.WithSize(24.0f).WithWeight(FontWeight::Bold); }
        [[nodiscard]] static TextStyle Body() { return TextStyle{}.WithSize(14.0f).WithWeight(FontWeight::Normal); }
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 4: PAINT CLASS
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class Paint {
    public:
        Paint();
        ~Paint();
        Paint(const Paint& other);
        Paint(Paint&&) noexcept;
        Paint& operator=(const Paint& other);
        Paint& operator=(Paint&&) noexcept;

        [[nodiscard]] static Paint Fill(const Color& color);
        [[nodiscard]] static Paint Stroke(const Color& color, float width = 1.0f);
        [[nodiscard]] static Paint LinearGradient(const Point& start, const Point& end, const std::vector<GradientStop>& stops);
        [[nodiscard]] static Paint RadialGradient(const Point& center, float radiusX, float radiusY, const std::vector<GradientStop>& stops);
        [[nodiscard]] static Paint RadialGradient(const Point& center, float radius, const std::vector<GradientStop>& stops);

        Paint& WithStyle(PaintStyle style);
        Paint& WithColor(const Color& color);
        Paint& WithAlpha(float alpha);
        Paint& WithStrokeWidth(float width);
        Paint& WithStrokeCap(StrokeCap cap);
        Paint& WithStrokeJoin(StrokeJoin join);
        Paint& WithMiterLimit(float limit);
        Paint& WithStrokeOptions(const StrokeOptions& options);

        [[nodiscard]] PaintStyle GetStyle() const;
        [[nodiscard]] BrushType GetBrushType() const;
        [[nodiscard]] Color GetColor() const;
        [[nodiscard]] float GetStrokeWidth() const;
        [[nodiscard]] float GetAlpha() const;
        [[nodiscard]] StrokeOptions GetStrokeOptions() const;
        [[nodiscard]] const std::vector<GradientStop>& GetGradientStops() const;
        [[nodiscard]] Point GetLinearStart() const;
        [[nodiscard]] Point GetLinearEnd() const;
        [[nodiscard]] Point GetRadialCenter() const;
        [[nodiscard]] float GetRadialRadiusX() const;
        [[nodiscard]] float GetRadialRadiusY() const;

        [[nodiscard]] bool IsFilled() const;
        [[nodiscard]] bool IsStroked() const;
        [[nodiscard]] bool IsGradient() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 5: GRAPHICS CORE
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class GraphicsCore final {
    public:
        class TransformScope {
        public:
            explicit TransformScope(GraphicsCore* core);
            ~TransformScope() noexcept;
            TransformScope(TransformScope&& other) noexcept;
            TransformScope& operator=(TransformScope&& other) noexcept;
            TransformScope(const TransformScope&) = delete;
            TransformScope& operator=(const TransformScope&) = delete;
        private:
            GraphicsCore* m_core;
            bool m_active;
        };

        GraphicsCore();
        ~GraphicsCore() noexcept;
        GraphicsCore(const GraphicsCore&) = delete;
        GraphicsCore& operator=(const GraphicsCore&) = delete;
        GraphicsCore(GraphicsCore&&) = delete;
        GraphicsCore& operator=(GraphicsCore&&) = delete;

        bool InitializeD2D(HWND hwnd, WindowMode mode);
        bool InitializeD3D11(HWND hwnd);
        void Shutdown() noexcept;
        bool RecreateResources(int width, int height);

        bool BeginDraw();
        HRESULT EndDraw();
        void Clear(const Color& color);
        [[nodiscard]] bool IsDrawing() const noexcept;

        void PushTransform();
        void PopTransform();
        void Rotate(const Point& center, float degrees);
        void Scale(const Point& center, float sx, float sy);
        void Translate(float dx, float dy);
        void SetTransform(const D2D1_MATRIX_3X2_F& matrix);
        void ResetTransform();

        ID2D1SolidColorBrush* GetSolidBrush(const Color& color);
        ID2D1LinearGradientBrush* GetLinearGradient(const Point& start, const Point& end, const std::vector<GradientStop>& stops);
        ID2D1RadialGradientBrush* GetRadialGradient(const Point& center, float radiusX, float radiusY, const std::vector<GradientStop>& stops);
        ID2D1Brush* GetBrushFromPaint(const Paint& paint, float globalAlpha = 1.0f);

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreatePathGeometry(std::function<void(ID2D1GeometrySink*)> buildFunc);

        void BeginOpacityLayer(float opacity);
        void EndOpacityLayer();
        void PushClipRect(const Rect& rect);
        void PopClipRect();
        void ClearCache();

        [[nodiscard]] ID2D1Factory* GetFactory() const noexcept;
        [[nodiscard]] IDWriteFactory* GetDWriteFactory() const noexcept;
        [[nodiscard]] ID2D1RenderTarget* GetRenderTarget() const noexcept;
        [[nodiscard]] ID3D11Device* GetD3D11Device() const noexcept;
        [[nodiscard]] ID3D11DeviceContext* GetD3D11Context() const noexcept;
        [[nodiscard]] ID3D11RenderTargetView* GetD3D11RenderTargetView() const noexcept;
        [[nodiscard]] IDXGISwapChain* GetSwapChain() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 6: GEOMETRY BUILDER
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class GeometryBuilder final {
    public:
        explicit GeometryBuilder(ID2D1Factory* factory);

        GeometryBuilder(const GeometryBuilder&) = delete;
        GeometryBuilder& operator=(const GeometryBuilder&) = delete;
        GeometryBuilder(GeometryBuilder&&) = delete;
        GeometryBuilder& operator=(GeometryBuilder&&) = delete;

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreatePathFromPoints(const std::vector<Point>& points, bool closed, bool filled) const;
        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateArc(const Point& center, float radius, float startAngle, float sweepAngle) const;
        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateRegularPolygon(const Point& center, float radius, int sides, float rotation = 0.0f) const;
        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateAngularSlice(const Point& center, float radius, float startAngle, float endAngle) const;

        [[nodiscard]] static std::vector<Point> GenerateCirclePoints(const Point& center, float radius, int segments);
        [[nodiscard]] static std::vector<Point> GenerateStarVertices(const Point& center, float outerRadius, float innerRadius, int points);
        [[nodiscard]] static std::vector<Point> GenerateRegularPolygonVertices(const Point& center, float radius, int sides, float rotation = 0.0f);
        [[nodiscard]] static std::vector<Point> GenerateWaveformPoints(const SpectrumData& spectrum, const Rect& bounds);

    private:
        ID2D1Factory* m_factory;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 7: RENDER ENGINE
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class RenderEngine final {
    public:
        class DrawScope final {
        public:
            explicit DrawScope(RenderEngine& engine);
            ~DrawScope() noexcept;
            DrawScope(const DrawScope&) = delete;
            DrawScope& operator=(const DrawScope&) = delete;
            DrawScope(DrawScope&&) = delete;
            DrawScope& operator=(DrawScope&&) = delete;
            [[nodiscard]] bool IsActive() const noexcept;
        private:
            RenderEngine& m_engine;
            bool m_begun;
        };

        explicit RenderEngine(HWND hwnd, WindowMode windowMode = WindowMode::Normal, RenderMode renderMode = RenderMode::Direct2D);

        // Backward compatibility constructor
        explicit RenderEngine(HWND hwnd, bool isOverlay, bool useD2DOnly)
            : RenderEngine(hwnd,
                isOverlay ? WindowMode::Overlay : WindowMode::Normal,
                useD2DOnly ? RenderMode::Direct2D : RenderMode::Direct3D11) {
        }

        ~RenderEngine() noexcept;

        RenderEngine(const RenderEngine&) = delete;
        RenderEngine& operator=(const RenderEngine&) = delete;
        RenderEngine(RenderEngine&&) = delete;
        RenderEngine& operator=(RenderEngine&&) = delete;

        [[nodiscard]] bool Initialize();
        void Resize(int width, int height);

        [[nodiscard]] bool BeginDraw();
        [[nodiscard]] HRESULT EndDraw();
        [[nodiscard]] DrawScope CreateDrawScope();
        void Clear(const Color& color);

        void ClearD3D11(const Color& color);
        void Present();

        [[nodiscard]] class Canvas& GetCanvas();
        [[nodiscard]] const class Canvas& GetCanvas() const;
        [[nodiscard]] GraphicsCore& GetCore() noexcept;
        [[nodiscard]] const GraphicsCore& GetCore() const noexcept;

        [[nodiscard]] ID3D11Device* GetD3D11Device() const noexcept;
        [[nodiscard]] ID3D11DeviceContext* GetD3D11DeviceContext() const noexcept;
        [[nodiscard]] ID3D11RenderTargetView* GetD3D11RenderTargetView() const noexcept;

        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;
        [[nodiscard]] bool IsDrawing() const noexcept;
        [[nodiscard]] RenderMode GetRenderMode() const noexcept;

        // Backward compatibility helpers
        [[nodiscard]] bool IsD2DMode() const noexcept { return GetRenderMode() == RenderMode::Direct2D; }
        [[nodiscard]] bool IsD3D11Mode() const noexcept { return GetRenderMode() == RenderMode::Direct3D11; }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 8: RENDERER
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class Renderer final {
    public:
        Renderer(ID2D1Factory* d2dFactory, IDWriteFactory* writeFactory);
        ~Renderer() noexcept;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        void SetRenderTarget(ID2D1RenderTarget* renderTarget);
        void OnDeviceLost();

        void DrawText(const std::wstring& text, const Rect& rect, const TextStyle& style);

        void DrawRectangle(const Rect& rect, const Paint& paint);
        void DrawRoundedRectangle(const Rect& rect, float radius, const Paint& paint);
        void DrawEllipse(const Point& center, float radiusX, float radiusY, const Paint& paint);
        void DrawLine(const Point& start, const Point& end, const Paint& paint);
        void DrawGeometry(ID2D1Geometry* geometry, const Paint& paint);
        void FillGeometry(ID2D1Geometry* geometry, const Paint& paint);

        ID2D1Brush* GetBrush(const Paint& paint);
        ID2D1SolidColorBrush* GetSolidBrush(const Color& color);

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreatePath(const std::vector<Point>& points, bool closed);
        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreatePathFromLines(const std::vector<Point>& points);

        [[nodiscard]] ID2D1Factory* GetFactory() const noexcept;
        [[nodiscard]] ID2D1RenderTarget* GetRenderTarget() const noexcept;
        [[nodiscard]] IDWriteFactory* GetWriteFactory() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // SECTION 9: CANVAS
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class Canvas final {
    public:
        Canvas(Renderer* renderer, GraphicsCore* core);
        ~Canvas() noexcept;

        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;
        Canvas(Canvas&&) = delete;
        Canvas& operator=(Canvas&&) = delete;

        [[nodiscard]] ID2D1RenderTarget* GetRenderTarget() const noexcept;

        // Basic primitives
        void DrawRectangle(const Rect& rect, const Paint& paint) const;
        void DrawRoundedRectangle(const Rect& rect, float radius, const Paint& paint) const;
        void DrawCircle(const Point& center, float radius, const Paint& paint) const;
        void DrawEllipse(const Point& center, float radiusX, float radiusY, const Paint& paint) const;
        void DrawLine(const Point& start, const Point& end, const Paint& paint) const;
        void DrawPolyline(const std::vector<Point>& points, const Paint& paint) const;
        void DrawPolygon(const std::vector<Point>& points, const Paint& paint) const;

        // Complex shapes
        void DrawArc(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const;
        void DrawRing(const Point& center, float innerRadius, float outerRadius, const Paint& paint) const;
        void DrawSector(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const;
        void DrawRegularPolygon(const Point& center, float radius, int sides, float rotation, const Paint& paint) const;
        void DrawStar(const Point& center, float outerRadius, float innerRadius, int points, const Paint& paint) const;
        void DrawGrid(const Rect& bounds, int rows, int cols, const Paint& paint) const;

        // Effects
        void DrawGlow(const Point& center, float radius, const Color& glowColor, float intensity = 1.0f, int layers = Constants::Effects::kDefaultGlowLayers) const;
        void DrawRectangleGlow(const Rect& rect, const Color& glowColor, float intensity = 1.0f, int layers = Constants::Effects::kDefaultGlowLayers) const;
        void DrawRoundedRectangleGlow(const Rect& rect, float cornerRadius, const Color& glowColor, float intensity = 1.0f, int layers = Constants::Effects::kDefaultGlowLayers) const;
        void DrawWithShadow(std::function<void()> drawCallback, const Point& offset, float blur, const Color& shadowColor) const;

        // Batch rendering
        void DrawCircleBatch(const std::vector<Point>& centers, float radius, const Paint& paint) const;
        void DrawRectangleBatch(const std::vector<Rect>& rects, const Paint& paint) const;

        // Layers and clipping
        void BeginOpacityLayer(float opacity) const;
        void EndOpacityLayer() const;
        void PushClipRect(const Rect& rect) const;
        void PopClipRect() const;

        // Transforms
        void PushTransform() const;
        void PopTransform() const;
        void RotateAt(const Point& center, float angleDegrees) const;
        void ScaleAt(const Point& center, float scaleX, float scaleY) const;
        void TranslateBy(float dx, float dy) const;
        void SetTransform(const D2D1_MATRIX_3X2_F& transform) const;
        void ResetTransform() const;

        // Text
        void DrawText(const std::wstring& text, const Rect& layoutRect, const TextStyle& style) const;
        void DrawText(const std::wstring& text, const Point& position, const TextStyle& style) const;

        // Spectrum visualization
        void DrawSpectrumBars(const SpectrumData& spectrum, const Rect& bounds, const BarStyle& style, const Color& color) const;
        void DrawWaveform(const SpectrumData& spectrum, const Rect& bounds, const Paint& paint, bool mirror = false) const;

    private:
        Renderer* m_renderer;
        GraphicsCore* m_core;
    };

} // namespace Spectrum

#endif // SPECTRUM_GRAPHICS_API_H