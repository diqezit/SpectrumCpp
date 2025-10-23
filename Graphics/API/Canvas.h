#ifndef SPECTRUM_CPP_CANVAS_H
#define SPECTRUM_CPP_CANVAS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the Canvas, a stateless facade for all 2D drawing operations.
//
// This class serves as the primary drawing interface for the application,
// abstracting away the underlying renderer implementations. It is designed to be
// ideologically similar to modern 2D graphics APIs like Skia's SKCanvas or
// C#'s Graphics object.
//
// Key features:
// - Provides a unified, "clean" API for drawing primitives, text, etc.
// - Is completely stateless regarding D2D resources; it only delegates calls.
// - Uses the 'Paint' and 'TextStyle' classes to simplify method signatures.
// - Does not manage resource lifetimes; it borrows non-owning pointers from RenderEngine.
//
// Architecture pattern: Facade
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/SpectrumTypes.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <d2d1.h>
#include <dwrite.h>
#include <functional>
#include <vector>

namespace Spectrum {

    // Forward-declarations to avoid including heavy headers.
    class PrimitiveRenderer;
    class TextRenderer;
    class EffectsRenderer;
    class TransformManager;
    class SpectrumRenderer;
    class Paint;
    struct TextStyle;

    class Canvas final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Constructor
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Canvas(
            PrimitiveRenderer* primitiveRenderer,
            TextRenderer* textRenderer,
            EffectsRenderer* effectsRenderer,
            TransformManager* transformManager,
            SpectrumRenderer* spectrumRenderer
        );

        // IRenderComponent implementation
        void OnRenderTargetChanged(
            ID2D1RenderTarget* renderTarget
        ) override;

        void OnDeviceLost() override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Access
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] ID2D1HwndRenderTarget* GetRenderTarget() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Primitives
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawRectangle(
            const Rect& rect,
            const Paint& paint
        ) const;

        void DrawRoundedRectangle(
            const Rect& rect,
            float radius,
            const Paint& paint
        ) const;

        void DrawCircle(
            const Point& center,
            float radius,
            const Paint& paint
        ) const;

        void DrawEllipse(
            const Point& center,
            float radiusX,
            float radiusY,
            const Paint& paint
        ) const;

        void DrawLine(
            const Point& start,
            const Point& end,
            const Paint& paint
        ) const;

        void DrawPolyline(
            const std::vector<Point>& points,
            const Paint& paint
        ) const;

        void DrawPolygon(
            const std::vector<Point>& points,
            const Paint& paint
        ) const;

        void DrawArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Paint& paint
        ) const;

        void DrawRing(
            const Point& center,
            float innerRadius,
            float outerRadius,
            const Paint& paint
        ) const;

        void DrawSector(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Paint& paint
        ) const;

        void DrawRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation,
            const Paint& paint
        ) const;

        void DrawStar(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points,
            const Paint& paint
        ) const;

        void DrawGrid(
            const Rect& bounds,
            int rows,
            int cols,
            const Paint& paint
        ) const;

        void DrawCircleBatch(
            const std::vector<Point>& centers,
            float radius,
            const Paint& paint
        ) const;

        void DrawRectangleBatch(
            const std::vector<Rect>& rects,
            const Paint& paint
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

        void RotateAt(
            const Point& center,
            float angleDegrees
        ) const;

        void ScaleAt(
            const Point& center,
            float scaleX,
            float scaleY
        ) const;

        void TranslateBy(
            float dx,
            float dy
        ) const;

        void SetTransform(const D2D1_MATRIX_3X2_F& transform) const;
        void ResetTransform() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Text
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawText(
            const std::wstring& text,
            const Rect& layoutRect,
            const TextStyle& style
        ) const;

        void DrawText(
            const std::wstring& text,
            const Point& position,
            const TextStyle& style
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
            const Paint& paint,
            bool mirror = false
        ) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables (Non-owning pointers)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1HwndRenderTarget* m_renderTarget;
        PrimitiveRenderer* m_primitiveRenderer;
        TextRenderer* m_textRenderer;
        EffectsRenderer* m_effectsRenderer;
        TransformManager* m_transformManager;
        SpectrumRenderer* m_spectrumRenderer;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CANVAS_H