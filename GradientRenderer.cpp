// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the GradientRenderer class
// It provides methods for drawing shapes with linear, radial, and
// simulated angular gradients to create rich visual styles
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GradientRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"

namespace Spectrum {

    namespace {
        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }

        inline D2D1_RECT_F ToD2DRect(const Rect& r) {
            return D2D1::RectF(r.x, r.y, r.GetRight(), r.GetBottom());
        }
    }

    GradientRenderer::GradientRenderer(
        ID2D1RenderTarget* renderTarget,
        ID2D1SolidColorBrush* solidBrush,
        ResourceCache* cache,
        GeometryBuilder* geometryBuilder
    )
        : m_renderTarget(renderTarget)
        , m_solidBrush(solidBrush)
        , m_cache(cache)
        , m_geometryBuilder(geometryBuilder)
    {
    }

    void GradientRenderer::DrawGradientRectangle(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool horizontal
    ) {
        if (!m_renderTarget || !m_cache || stops.empty()) {
            return;
        }

        Point start = { rect.x, rect.y };
        Point end = horizontal
            ? Point{ rect.GetRight(), rect.y }
        : Point{ rect.x, rect.GetBottom() };

        auto* brush = m_cache->GetLinearGradient("gradient_rect", start, end, stops);
        if (brush) {
            D2D1_RECT_F d2dRect = ToD2DRect(rect);
            m_renderTarget->FillRectangle(&d2dRect, brush);
        }
    }

    void GradientRenderer::DrawRadialGradient(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (!m_renderTarget || !m_cache || stops.empty()) {
            return;
        }

        auto* brush = m_cache->GetRadialGradient("radial_gradient", center, radius, stops);
        if (brush) {
            D2D1_ELLIPSE ellipse = {};
            ellipse.point = ToD2DPoint(center);
            ellipse.radiusX = radius;
            ellipse.radiusY = radius;

            m_renderTarget->FillEllipse(&ellipse, brush);
        }
    }

    // if user wants unfilled gradient circle, fall back to solid color outline
    // true gradient outlines are complex and not a core requirement
    void GradientRenderer::DrawGradientCircle(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool filled
    ) {
        if (filled) {
            DrawRadialGradient(center, radius, stops);
        }
        else if (!stops.empty()) {
            const auto& backColor = stops.back().color;
            Color c = { backColor.r, backColor.g, backColor.b, backColor.a };

            if (m_solidBrush) {
                m_solidBrush->SetColor(ToD2DColor(c));

                float stroke = 2.0f;
                D2D1_ELLIPSE ellipse = {};
                ellipse.point = ToD2DPoint(center);
                // adjust radius to account for stroke width
                ellipse.radiusX = radius - stroke / 2.0f;
                ellipse.radiusY = radius - stroke / 2.0f;

                m_renderTarget->DrawEllipse(&ellipse, m_solidBrush, stroke);
            }
        }
    }

    void GradientRenderer::DrawGradientPath(
        const std::vector<Point>& points,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_cache || !m_geometryBuilder ||
            points.size() < 2 || stops.empty()) {
            return;
        }

        auto* brush = m_cache->GetLinearGradient(
            "path_gradient",
            points.front(),
            points.back(),
            stops
        );

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);

        if (brush && geo) {
            m_renderTarget->DrawGeometry(geo.Get(), brush, strokeWidth);
        }
    }

    // D2D does not support angular gradients directly
    // simulate it by drawing many small, colored triangles (slices)
    void GradientRenderer::DrawAngularGradient(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle,
        const Color& startColor,
        const Color& endColor
    ) {
        // more segments provide a smoother gradient at higher cost
        const int segments = 180;
        float sweep = endAngle - startAngle;

        if (fabsf(sweep) < 0.1f || !m_renderTarget || !m_geometryBuilder || !m_solidBrush) {
            return;
        }

        float angleStep = sweep / static_cast<float>(segments);

        for (int i = 0; i < segments; ++i) {
            float a0 = startAngle + i * angleStep;
            float a1 = startAngle + (i + 1) * angleStep;

            auto geo = m_geometryBuilder->CreateAngularSlice(center, radius, a0, a1);
            if (geo) {
                // interpolate color for the middle of the slice
                Color midColor = Utils::InterpolateColor(
                    startColor,
                    endColor,
                    (static_cast<float>(i) + 0.5f) / segments
                );

                m_solidBrush->SetColor(ToD2DColor(midColor));
                m_renderTarget->FillGeometry(geo.Get(), m_solidBrush);
            }
        }
    }

    void GradientRenderer::DrawVerticalGradientBar(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float cornerRadius
    ) {
        if (!m_renderTarget || !m_cache || stops.empty()) {
            return;
        }

        auto* brush = m_cache->GetLinearGradient(
            "vbar_gradient",
            { rect.x, rect.y },
            { rect.x, rect.GetBottom() },
            stops
        );

        if (!brush) {
            return;
        }

        if (cornerRadius > 0.0f) {
            D2D1_ROUNDED_RECT rr = {
                ToD2DRect(rect),
                cornerRadius,
                cornerRadius
            };
            m_renderTarget->FillRoundedRectangle(&rr, brush);
        }
        else {
            D2D1_RECT_F d2dRect = ToD2DRect(rect);
            m_renderTarget->FillRectangle(&d2dRect, brush);
        }
    }

    void GradientRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;
    }

} // namespace Spectrum