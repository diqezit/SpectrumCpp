#ifndef SPECTRUM_CPP_GRAPHICS_CONTEXT_H
#define SPECTRUM_CPP_GRAPHICS_CONTEXT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the GraphicsContext, the central facade for rendering operations.
//
// This class serves as the main entry point for all drawing operations,
// managing Direct2D/DirectWrite resources and delegating to specialized
// renderer components. While it appears to violate SRP with 80+ methods,
// this is intentional as it implements the Facade pattern.
//
// Key responsibilities:
// - Direct2D/DirectWrite resource lifecycle management
// - Device lost scenario handling and recovery
// - Unified facade for all rendering operations
// - Automatic render target propagation to sub-components
//
// Design notes:
// - All delegation methods are const (stateless facade)
// - Resource management methods are non-const (modify internal state)
// - Uses unique_ptr for sub-components (ownership)
// - Non-owning HWND pointer (lifetime managed by OS)
//
// Architecture pattern: Facade + Dependency Injection
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "ResourceCache.h"
#include "GeometryBuilder.h"
#include "PrimitiveRenderer.h"
#include "GradientRenderer.h"
#include "TextRenderer.h"
#include "EffectsRenderer.h"
#include "TransformManager.h"
#include "SpectrumRenderer.h"
#include <memory>
#include <functional>

namespace Spectrum {

    class GraphicsContext final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // RAII Draw Scope
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        class DrawScope final
        {
        public:
            explicit DrawScope(GraphicsContext& context)
                : m_context(context)
            {
                m_context.BeginDraw();
            }

            ~DrawScope() noexcept
            {
                (void)m_context.EndDraw();
            }

            DrawScope(const DrawScope&) = delete;
            DrawScope& operator=(const DrawScope&) = delete;

        private:
            GraphicsContext& m_context;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit GraphicsContext(HWND hwnd);
        ~GraphicsContext();

        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;

        [[nodiscard]] bool Initialize();
        void Resize(int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing Control
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void BeginDraw();
        [[nodiscard]] HRESULT EndDraw();
        [[nodiscard]] DrawScope CreateDrawScope() { return DrawScope(*this); }
        void Clear(const Color& color) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Primitives
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawRectangle(
            const Rect& rect,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawRoundedRectangle(
            const Rect& rect,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawCircle(
            const Point& center,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawEllipse(
            const Point& center,
            float radiusX,
            float radiusY,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawLine(
            const Point& start,
            const Point& end,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        void DrawPolyline(
            const std::vector<Point>& points,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        void DrawPolygon(
            const std::vector<Point>& points,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        void DrawRing(
            const Point& center,
            float innerRadius,
            float outerRadius,
            const Color& color
        ) const;

        void DrawSector(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            bool filled = true
        ) const;

        void DrawRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawStar(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points,
            const Color& color,
            bool filled = true
        ) const;

        void DrawGrid(
            const Rect& bounds,
            int rows,
            int cols,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        void DrawCircleBatch(
            const std::vector<Point>& centers,
            float radius,
            const Color& color,
            bool filled = true
        ) const;

        void DrawRectangleBatch(
            const std::vector<Rect>& rects,
            const Color& color,
            bool filled = true
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Gradients
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawGradientRectangle(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool horizontal = true
        ) const;

        void DrawRadialGradient(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) const;

        void DrawGradientCircle(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool filled = true
        ) const;

        void DrawGradientPath(
            const std::vector<Point>& points,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float strokeWidth = 1.0f
        ) const;

        void DrawAngularGradient(
            const Point& center,
            float radius,
            float startAngle,
            float endAngle,
            const Color& startColor,
            const Color& endColor
        ) const;

        void DrawVerticalGradientBar(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float cornerRadius = 0.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Effects
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawWithShadow(
            std::function<void()> drawCallback,
            const Point& offset,
            float blur,
            const Color& shadowColor
        ) const;

        void DrawGlow(
            const Point& center,
            float radius,
            const Color& glowColor,
            float intensity = 1.0f
        ) const;

        void BeginOpacityLayer(float opacity) const;
        void EndOpacityLayer() const;

        void PushClipRect(const Rect& rect) const;
        void PopClipRect() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Transforms
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void PushTransform() const;
        void PopTransform() const;
        void RotateAt(const Point& center, float angleDegrees) const;
        void ScaleAt(const Point& center, float scaleX, float scaleY) const;
        void TranslateBy(float dx, float dy) const;
        void SetTransform(const D2D1_MATRIX_3X2_F& transform) const;
        void ResetTransform() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Text
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawText(
            const std::wstring& text,
            const Point& position,
            const Color& color,
            float fontSize = 12.0f,
            DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING
        ) const;

        void DrawTextWithOutline(
            const std::wstring& text,
            const Point& position,
            const Color& fillColor,
            const Color& outlineColor,
            float fontSize = 12.0f,
            float outlineWidth = 1.0f
        ) const;

        void DrawTextRotated(
            const std::wstring& text,
            const Point& position,
            float angleDegrees,
            const Color& color,
            float fontSize = 12.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Spectrum Visualization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawSpectrumBars(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const BarStyle& style,
            const Color& color
        ) const;

        void DrawWaveform(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float strokeWidth = 2.0f,
            bool mirror = false
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Access
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] ID2D1HwndRenderTarget* GetRenderTarget() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CreateD2DFactory();
        [[nodiscard]] bool CreateDWriteFactory();
        [[nodiscard]] bool CreateDeviceResources();
        [[nodiscard]] bool CreateHwndRenderTarget();
        [[nodiscard]] bool CreateSolidBrush();
        void DiscardDeviceResources();
        void UpdateComponentsRenderTarget();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        HWND m_hwnd;
        int m_width;
        int m_height;

        wrl::ComPtr<ID2D1Factory> m_d2dFactory;
        wrl::ComPtr<IDWriteFactory> m_writeFactory;
        wrl::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
        wrl::ComPtr<ID2D1SolidColorBrush> m_solidBrush;

        std::unique_ptr<ResourceCache> m_resourceCache;
        std::unique_ptr<GeometryBuilder> m_geometryBuilder;
        std::unique_ptr<PrimitiveRenderer> m_primitiveRenderer;
        std::unique_ptr<GradientRenderer> m_gradientRenderer;
        std::unique_ptr<TextRenderer> m_textRenderer;
        std::unique_ptr<EffectsRenderer> m_effectsRenderer;
        std::unique_ptr<TransformManager> m_transformManager;
        std::unique_ptr<SpectrumRenderer> m_spectrumRenderer;
    };

} // namespace Spectrum

#endif