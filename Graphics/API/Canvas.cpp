// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the Canvas facade by delegating all drawing calls to the
// specialized renderer components.
//
// Implementation details:
// - All methods are simple delegation calls to the appropriate renderer
// - Paint struct is unpacked into individual parameters for legacy renderers
// - Null-checks ensure safe operation even if a renderer is missing
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Canvas.h"

// Include the full definitions of the worker classes
#include "Renderers/PrimitiveRenderer.h"
#include "Renderers/GradientRenderer.h"
#include "Renderers/TextRenderer.h"
#include "Renderers/EffectsRenderer.h"
#include "Core/TransformManager.h"
#include "Renderers/SpectrumRenderer.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor and IRenderComponent Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Canvas::Canvas(
        PrimitiveRenderer* primitiveRenderer,
        GradientRenderer* gradientRenderer,
        TextRenderer* textRenderer,
        EffectsRenderer* effectsRenderer,
        TransformManager* transformManager,
        SpectrumRenderer* spectrumRenderer
    )
        : m_renderTarget(nullptr)
        , m_primitiveRenderer(primitiveRenderer)
        , m_gradientRenderer(gradientRenderer)
        , m_textRenderer(textRenderer)
        , m_effectsRenderer(effectsRenderer)
        , m_transformManager(transformManager)
        , m_spectrumRenderer(spectrumRenderer)
    {
    }

    void Canvas::OnRenderTargetChanged(ID2D1RenderTarget* renderTarget)
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

    void Canvas::DrawRectangle(const Rect& rect, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawRectangle(rect, paint.color, paint.isFilled, paint.strokeWidth); } }
    void Canvas::DrawRoundedRectangle(const Rect& rect, float radius, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawRoundedRectangle(rect, radius, paint.color, paint.isFilled, paint.strokeWidth); } }
    void Canvas::DrawCircle(const Point& center, float radius, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawCircle(center, radius, paint.color, paint.isFilled, paint.strokeWidth); } }
    void Canvas::DrawEllipse(const Point& center, float radiusX, float radiusY, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawEllipse(center, radiusX, radiusY, paint.color, paint.isFilled, paint.strokeWidth); } }
    void Canvas::DrawLine(const Point& start, const Point& end, const Color& color, float strokeWidth) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawLine(start, end, color, strokeWidth); } }
    void Canvas::DrawPolyline(const std::vector<Point>& points, const Color& color, float strokeWidth) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawPolyline(points, color, strokeWidth); } }
    void Canvas::DrawPolygon(const std::vector<Point>& points, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawPolygon(points, paint.color, paint.isFilled, paint.strokeWidth); } }
    void Canvas::DrawArc(const Point& center, float radius, float startAngle, float sweepAngle, const Color& color, float strokeWidth) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawArc(center, radius, startAngle, sweepAngle, color, strokeWidth); } }
    void Canvas::DrawRing(const Point& center, float innerRadius, float outerRadius, const Color& color) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawRing(center, innerRadius, outerRadius, color); } }
    void Canvas::DrawSector(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawSector(center, radius, startAngle, sweepAngle, paint.color, paint.isFilled); } }
    void Canvas::DrawRegularPolygon(const Point& center, float radius, int sides, float rotation, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawRegularPolygon(center, radius, sides, rotation, paint.color, paint.isFilled, paint.strokeWidth); } }
    void Canvas::DrawStar(const Point& center, float outerRadius, float innerRadius, int points, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawStar(center, outerRadius, innerRadius, points, paint.color, paint.isFilled); } }
    void Canvas::DrawGrid(const Rect& bounds, int rows, int cols, const Color& color, float strokeWidth) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawGrid(bounds, rows, cols, color, strokeWidth); } }
    void Canvas::DrawCircleBatch(const std::vector<Point>& centers, float radius, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawCircleBatch(centers, radius, paint.color, paint.isFilled); } }
    void Canvas::DrawRectangleBatch(const std::vector<Rect>& rects, const Paint& paint) const { if (m_primitiveRenderer) { m_primitiveRenderer->DrawRectangleBatch(rects, paint.color, paint.isFilled); } }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradients
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawGradientRectangle(const Rect& rect, const std::vector<D2D1_GRADIENT_STOP>& stops, bool horizontal) const { if (m_gradientRenderer) { m_gradientRenderer->DrawGradientRectangle(rect, stops, horizontal); } }
    void Canvas::DrawRadialGradient(const Point& center, float radius, const std::vector<D2D1_GRADIENT_STOP>& stops) const { if (m_gradientRenderer) { m_gradientRenderer->DrawRadialGradient(center, radius, stops); } }
    void Canvas::DrawGradientCircle(const Point& center, float radius, const std::vector<D2D1_GRADIENT_STOP>& stops, bool filled) const { if (m_gradientRenderer) { m_gradientRenderer->DrawGradientCircle(center, radius, stops, filled); } }
    void Canvas::DrawGradientPath(const std::vector<Point>& points, const std::vector<D2D1_GRADIENT_STOP>& stops, float strokeWidth) const { if (m_gradientRenderer) { m_gradientRenderer->DrawGradientPath(points, stops, strokeWidth); } }
    void Canvas::DrawAngularGradient(const Point& center, float radius, float startAngle, float endAngle, const Color& startColor, const Color& endColor) const { if (m_gradientRenderer) { m_gradientRenderer->DrawAngularGradient(center, radius, startAngle, endAngle, startColor, endColor); } }
    void Canvas::DrawVerticalGradientBar(const Rect& rect, const std::vector<D2D1_GRADIENT_STOP>& stops, float cornerRadius) const { if (m_gradientRenderer) { m_gradientRenderer->DrawVerticalGradientBar(rect, stops, cornerRadius); } }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Effects
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawWithShadow(std::function<void()> drawCallback, const Point& offset, float blur, const Color& shadowColor) const { if (m_effectsRenderer) { m_effectsRenderer->DrawWithShadow(drawCallback, offset, blur, shadowColor); } }
    void Canvas::DrawGlow(const Point& center, float radius, const Color& glowColor, float intensity) const { if (m_effectsRenderer) { m_effectsRenderer->DrawGlow(center, radius, glowColor, intensity); } }
    void Canvas::BeginOpacityLayer(float opacity) const { if (m_effectsRenderer) { m_effectsRenderer->BeginOpacityLayer(opacity); } }
    void Canvas::EndOpacityLayer() const { if (m_effectsRenderer) { m_effectsRenderer->EndOpacityLayer(); } }
    void Canvas::PushClipRect(const Rect& rect) const { if (m_effectsRenderer) { m_effectsRenderer->PushClipRect(rect); } }
    void Canvas::PopClipRect() const { if (m_effectsRenderer) { m_effectsRenderer->PopClipRect(); } }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transforms
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::PushTransform() const { if (m_transformManager) { m_transformManager->PushTransform(); } }
    void Canvas::PopTransform() const { if (m_transformManager) { m_transformManager->PopTransform(); } }
    void Canvas::RotateAt(const Point& center, float angleDegrees) const { if (m_transformManager) { m_transformManager->RotateAt(center, angleDegrees); } }
    void Canvas::ScaleAt(const Point& center, float scaleX, float scaleY) const { if (m_transformManager) { m_transformManager->ScaleAt(center, scaleX, scaleY); } }
    void Canvas::TranslateBy(float dx, float dy) const { if (m_transformManager) { m_transformManager->TranslateBy(dx, dy); } }
    void Canvas::SetTransform(const D2D1_MATRIX_3X2_F& transform) const { if (m_transformManager) { m_transformManager->SetTransform(transform); } }
    void Canvas::ResetTransform() const { if (m_transformManager) { m_transformManager->ResetTransform(); } }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawText(const std::wstring& text, const Point& position, const Color& color, float fontSize, DWRITE_TEXT_ALIGNMENT alignment) const { if (m_textRenderer) { m_textRenderer->DrawText(text, position, color, fontSize, alignment); } }
    void Canvas::DrawTextWithOutline(const std::wstring& text, const Point& position, const Color& fillColor, const Color& outlineColor, float fontSize, float outlineWidth) const { if (m_textRenderer) { m_textRenderer->DrawTextWithOutline(text, position, fillColor, outlineColor, fontSize, outlineWidth); } }
    void Canvas::DrawTextRotated(const std::wstring& text, const Point& position, float angleDegrees, const Color& color, float fontSize) const { if (m_textRenderer) { m_textRenderer->DrawTextRotated(text, position, angleDegrees, color, fontSize); } }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Visualization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void Canvas::DrawSpectrumBars(const SpectrumData& spectrum, const Rect& bounds, const BarStyle& style, const Color& color) const { if (m_spectrumRenderer) { m_spectrumRenderer->DrawSpectrumBars(spectrum, bounds, style, color); } }
    void Canvas::DrawWaveform(const SpectrumData& spectrum, const Rect& bounds, const Color& color, float strokeWidth, bool mirror) const { if (m_spectrumRenderer) { m_spectrumRenderer->DrawWaveform(spectrum, bounds, color, strokeWidth, mirror); } }

} // namespace Spectrum