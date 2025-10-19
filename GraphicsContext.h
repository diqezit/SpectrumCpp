#ifndef SPECTRUM_CPP_GRAPHICS_CONTEXT_H
#define SPECTRUM_CPP_GRAPHICS_CONTEXT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the GraphicsContext, the central facade for the
// entire rendering system. It manages the lifecycle of D2D/DWrite resources
// and provides a unified API for all drawing operations
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

namespace Spectrum {

    class GraphicsContext {
    public:
        explicit GraphicsContext(HWND hwnd);
        ~GraphicsContext();

        // Lifecycle management
        bool Initialize();
        void BeginDraw();
        HRESULT EndDraw();
        void Resize(int width, int height);
        void Clear(const Color& color);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Primitives
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawRectangle(
            const Rect& rect,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawRoundedRectangle(
            const Rect& rect,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawCircle(
            const Point& center,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawEllipse(
            const Point& center,
            float radiusX,
            float radiusY,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawLine(
            const Point& start,
            const Point& end,
            const Color& color,
            float strokeWidth = 1.0f
        );

        void DrawPolyline(
            const std::vector<Point>& points,
            const Color& color,
            float strokeWidth = 1.0f
        );

        void DrawPolygon(
            const std::vector<Point>& points,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            float strokeWidth = 1.0f
        );

        void DrawRing(
            const Point& center,
            float innerRadius,
            float outerRadius,
            const Color& color
        );

        void DrawSector(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            bool filled = true
        );

        void DrawRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawStar(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points,
            const Color& color,
            bool filled = true
        );

        void DrawGrid(
            const Rect& bounds,
            int rows,
            int cols,
            const Color& color,
            float strokeWidth = 1.0f
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Gradients
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawGradientRectangle(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool horizontal = true
        );

        void DrawRadialGradient(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        );

        void DrawGradientCircle(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool filled = true
        );

        void DrawGradientPath(
            const std::vector<Point>& points,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float strokeWidth = 1.0f
        );

        void DrawAngularGradient(
            const Point& center,
            float radius,
            float startAngle,
            float endAngle,
            const Color& startColor,
            const Color& endColor
        );

        void DrawVerticalGradientBar(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float cornerRadius = 0.0f
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Effects
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawWithShadow(
            std::function<void()> drawCallback,
            const Point& offset,
            float blur,
            const Color& shadowColor
        );

        void DrawGlow(
            const Point& center,
            float radius,
            const Color& glowColor,
            float intensity = 1.0f
        );

        void BeginOpacityLayer(float opacity);
        void EndOpacityLayer();

        void PushClipRect(const Rect& rect);
        void PopClipRect();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Transforms
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void PushTransform();
        void PopTransform();
        void RotateAt(const Point& center, float angleDegrees);
        void ScaleAt(const Point& center, float scaleX, float scaleY);
        void TranslateBy(float dx, float dy);
        void SetTransform(const D2D1_MATRIX_3X2_F& transform);
        void ResetTransform();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Text
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawText(
            const std::wstring& text,
            const Point& position,
            const Color& color,
            float fontSize = 12.0f,
            DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING
        );

        void DrawTextWithOutline(
            const std::wstring& text,
            const Point& position,
            const Color& fillColor,
            const Color& outlineColor,
            float fontSize = 12.0f,
            float outlineWidth = 1.0f
        );

        void DrawTextRotated(
            const std::wstring& text,
            const Point& position,
            float angleDegrees,
            const Color& color,
            float fontSize = 12.0f
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Batch Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawCircleBatch(
            const std::vector<Point>& centers,
            float radius,
            const Color& color,
            bool filled = true
        );

        void DrawRectangleBatch(
            const std::vector<Rect>& rects,
            const Color& color,
            bool filled = true
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Spectrum Visualization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawSpectrumBars(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const BarStyle& style,
            const Color& color
        );

        void DrawWaveform(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float strokeWidth = 2.0f,
            bool mirror = false
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Getters for low-level access
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        ID2D1HwndRenderTarget* GetRenderTarget() const noexcept;
        int GetWidth() const noexcept;
        int GetHeight() const noexcept;

    private:
        bool CreateD2DFactory();
        bool CreateDWriteFactory();
        bool CreateDeviceResources();
        bool CreateHwndRenderTarget();
        bool CreateSolidBrush();
        void DiscardDeviceResources();
        void UpdateComponentsRenderTarget();

        HWND m_hwnd;
        int m_width;
        int m_height;

        // Core D2D/DWrite objects
        wrl::ComPtr<ID2D1Factory> m_d2dFactory;
        wrl::ComPtr<IDWriteFactory> m_writeFactory;
        wrl::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
        wrl::ComPtr<ID2D1SolidColorBrush> m_solidBrush;

        // Specialized components
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