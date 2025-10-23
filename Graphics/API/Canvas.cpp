// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the Canvas facade by delegating all drawing calls to the
// specialized renderer components.
//
// Implementation details:
// - All methods are simple delegation calls to the appropriate renderer
// - Paint and TextStyle classes are used to simplify method signatures
// - Null-checks ensure safe operation even if a renderer is missing
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Graphics/API/Renderers/PrimitiveRenderer.h"
#include "Graphics/API/Renderers/TextRenderer.h"
#include "Graphics/API/Renderers/EffectsRenderer.h"
#include "Graphics/API/Core/TransformManager.h"
#include "Graphics/API/Renderers/SpectrumRenderer.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor and IRenderComponent Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Canvas::Canvas(
        PrimitiveRenderer* primitiveRenderer,
        TextRenderer* textRenderer,
        EffectsRenderer* effectsRenderer,
        TransformManager* transformManager,
        SpectrumRenderer* spectrumRenderer
    )
        : m_renderTarget(nullptr)
        , m_primitiveRenderer(primitiveRenderer)
        , m_textRenderer(textRenderer)
        , m_effectsRenderer(effectsRenderer)
        , m_transformManager(transformManager)
        , m_spectrumRenderer(spectrumRenderer)
    {
    }

    void Canvas::OnRenderTargetChanged(
        ID2D1RenderTarget* renderTarget
    )
    {
        m_renderTarget = static_cast<ID2D1HwndRenderTarget*>(renderTarget);
    }

    void Canvas::OnDeviceLost()
    {
        m_renderTarget = nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Access
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ID2D1HwndRenderTarget* Canvas::GetRenderTarget() const noexcept
    {
        return m_renderTarget;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Primitives
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawRectangle(
        const Rect& rect,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRectangle(rect, paint);
        }
    }

    void Canvas::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRoundedRectangle(rect, radius, paint);
        }
    }

    void Canvas::DrawCircle(
        const Point& center,
        float radius,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawCircle(center, radius, paint);
        }
    }

    void Canvas::DrawEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawEllipse(center, radiusX, radiusY, paint);
        }
    }

    void Canvas::DrawLine(
        const Point& start,
        const Point& end,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawLine(start, end, paint);
        }
    }

    void Canvas::DrawPolyline(
        const std::vector<Point>& points,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawPolyline(points, paint);
        }
    }

    void Canvas::DrawPolygon(
        const std::vector<Point>& points,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawPolygon(points, paint);
        }
    }

    void Canvas::DrawArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawArc(center, radius, startAngle, sweepAngle, paint);
        }
    }

    void Canvas::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRing(center, innerRadius, outerRadius, paint);
        }
    }

    void Canvas::DrawSector(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawSector(center, radius, startAngle, sweepAngle, paint);
        }
    }

    void Canvas::DrawRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRegularPolygon(center, radius, sides, rotation, paint);
        }
    }

    void Canvas::DrawStar(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawStar(center, outerRadius, innerRadius, points, paint);
        }
    }

    void Canvas::DrawGrid(
        const Rect& bounds,
        int rows,
        int cols,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawGrid(bounds, rows, cols, paint);
        }
    }

    void Canvas::DrawCircleBatch(
        const std::vector<Point>& centers,
        float radius,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawCircleBatch(centers, radius, paint);
        }
    }

    void Canvas::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Paint& paint
    ) const {
        if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRectangleBatch(rects, paint);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Effects
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawWithShadow(
        std::function<void()> drawCallback,
        const Point& offset,
        float blur,
        const Color& shadowColor
    ) const {
        if (m_effectsRenderer) {
            m_effectsRenderer->DrawWithShadow(drawCallback, offset, blur, shadowColor);
        }
    }

    void Canvas::DrawGlow(
        const Point& center,
        float radius,
        const Color& glowColor,
        float intensity
    ) const {
        if (m_effectsRenderer) {
            m_effectsRenderer->DrawGlow(center, radius, glowColor, intensity);
        }
    }

    void Canvas::BeginOpacityLayer(float opacity) const {
        if (m_effectsRenderer) {
            m_effectsRenderer->BeginOpacityLayer(opacity);
        }
    }

    void Canvas::EndOpacityLayer() const {
        if (m_effectsRenderer) {
            m_effectsRenderer->EndOpacityLayer();
        }
    }

    void Canvas::PushClipRect(const Rect& rect) const {
        if (m_effectsRenderer) {
            m_effectsRenderer->PushClipRect(rect);
        }
    }

    void Canvas::PopClipRect() const {
        if (m_effectsRenderer) {
            m_effectsRenderer->PopClipRect();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transforms
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::PushTransform() const {
        if (m_transformManager) {
            m_transformManager->PushTransform();
        }
    }

    void Canvas::PopTransform() const {
        if (m_transformManager) {
            m_transformManager->PopTransform();
        }
    }

    void Canvas::RotateAt(
        const Point& center,
        float angleDegrees
    ) const {
        if (m_transformManager) {
            m_transformManager->RotateAt(center, angleDegrees);
        }
    }

    void Canvas::ScaleAt(
        const Point& center,
        float scaleX,
        float scaleY
    ) const {
        if (m_transformManager) {
            m_transformManager->ScaleAt(center, scaleX, scaleY);
        }
    }

    void Canvas::TranslateBy(
        float dx,
        float dy
    ) const {
        if (m_transformManager) {
            m_transformManager->TranslateBy(dx, dy);
        }
    }

    void Canvas::SetTransform(const D2D1_MATRIX_3X2_F& transform) const {
        if (m_transformManager) {
            m_transformManager->SetTransform(transform);
        }
    }

    void Canvas::ResetTransform() const {
        if (m_transformManager) {
            m_transformManager->ResetTransform();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawText(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const {
        if (m_textRenderer) {
            m_textRenderer->DrawText(text, layoutRect, style);
        }
    }

    void Canvas::DrawText(
        const std::wstring& text,
        const Point& position,
        const TextStyle& style
    ) const {
        if (m_textRenderer) {
            m_textRenderer->DrawText(text, position, style);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Visualization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawSpectrumBars(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const BarStyle& style,
        const Color& color
    ) const {
        if (m_spectrumRenderer) {
            m_spectrumRenderer->DrawSpectrumBars(spectrum, bounds, style, color);
        }
    }

    void Canvas::DrawWaveform(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Paint& paint,
        bool mirror
    ) const {
        if (m_spectrumRenderer) {
            m_spectrumRenderer->DrawWaveform(spectrum, bounds, paint, mirror);
        }
    }
}