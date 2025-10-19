// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the PrimitiveRenderer class
// It wraps Direct2D's drawing functions to provide a simplified,
// consistent API for drawing shapes with solid colors
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "PrimitiveRenderer.h"
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

    PrimitiveRenderer::PrimitiveRenderer(
        ID2D1RenderTarget* renderTarget,
        ID2D1SolidColorBrush* brush,
        GeometryBuilder* geometryBuilder
    )
        : m_renderTarget(renderTarget)
        , m_brush(brush)
        , m_geometryBuilder(geometryBuilder)
    {
    }

    // set solid brush color before a drawing operation
    void PrimitiveRenderer::SetBrushColor(const Color& color) {
        if (m_brush) {
            m_brush->SetColor(ToD2DColor(color));
        }
    }

    void PrimitiveRenderer::DrawRectangle(
        const Rect& rect,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_brush) {
            return;
        }

        SetBrushColor(color);
        D2D1_RECT_F d2dRect = ToD2DRect(rect);

        if (filled) {
            m_renderTarget->FillRectangle(&d2dRect, m_brush);
        }
        else {
            m_renderTarget->DrawRectangle(&d2dRect, m_brush, strokeWidth);
        }
    }

    void PrimitiveRenderer::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_brush) {
            return;
        }

        SetBrushColor(color);
        D2D1_ROUNDED_RECT rr = { ToD2DRect(rect), radius, radius };

        if (filled) {
            m_renderTarget->FillRoundedRectangle(&rr, m_brush);
        }
        else {
            m_renderTarget->DrawRoundedRectangle(&rr, m_brush, strokeWidth);
        }
    }

    // circle is a special case of an ellipse
    void PrimitiveRenderer::DrawCircle(
        const Point& center,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        DrawEllipse(center, radius, radius, color, filled, strokeWidth);
    }

    void PrimitiveRenderer::DrawEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_brush) {
            return;
        }

        SetBrushColor(color);
        D2D1_ELLIPSE ellipse = {};
        ellipse.point = ToD2DPoint(center);
        ellipse.radiusX = radiusX;
        ellipse.radiusY = radiusY;

        if (filled) {
            m_renderTarget->FillEllipse(&ellipse, m_brush);
        }
        else {
            m_renderTarget->DrawEllipse(&ellipse, m_brush, strokeWidth);
        }
    }

    void PrimitiveRenderer::DrawLine(
        const Point& start,
        const Point& end,
        const Color& color,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_brush) {
            return;
        }

        SetBrushColor(color);
        m_renderTarget->DrawLine(
            ToD2DPoint(start),
            ToD2DPoint(end),
            m_brush,
            strokeWidth
        );
    }

    // delegate path creation to geometry builder
    void PrimitiveRenderer::DrawPolyline(
        const std::vector<Point>& points,
        const Color& color,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_geometryBuilder) {
            return;
        }

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);
        if (geo) {
            SetBrushColor(color);
            m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
        }
    }

    void PrimitiveRenderer::DrawPolygon(
        const std::vector<Point>& points,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_geometryBuilder) {
            return;
        }

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, true, filled);
        if (geo) {
            SetBrushColor(color);
            if (filled) {
                m_renderTarget->FillGeometry(geo.Get(), m_brush);
            }
            else {
                m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
            }
        }
    }

    void PrimitiveRenderer::DrawArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Color& color,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_geometryBuilder || sweepAngle == 0.0f) {
            return;
        }

        auto geo = m_geometryBuilder->CreateArc(center, radius, startAngle, sweepAngle);
        if (geo) {
            SetBrushColor(color);
            m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
        }
    }

    // simulate a ring by drawing a thick-stroked circle
    // this avoids creating a complex donut-shaped geometry
    void PrimitiveRenderer::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Color& color
    ) {
        if (!m_renderTarget || innerRadius >= outerRadius) {
            return;
        }

        float stroke = outerRadius - innerRadius;
        float radius = innerRadius + stroke / 2.0f;
        DrawCircle(center, radius, color, false, stroke);
    }

    void PrimitiveRenderer::DrawSector(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Color& color,
        bool filled
    ) {
        if (!m_renderTarget || !m_geometryBuilder || sweepAngle == 0.0f) {
            return;
        }

        auto geo = m_geometryBuilder->CreateAngularSlice(
            center, radius, startAngle, startAngle + sweepAngle
        );

        if (geo) {
            SetBrushColor(color);
            if (filled) {
                m_renderTarget->FillGeometry(geo.Get(), m_brush);
            }
            else {
                m_renderTarget->DrawGeometry(geo.Get(), m_brush, 1.0f);
            }
        }
    }

    void PrimitiveRenderer::DrawRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation,
        const Color& color,
        bool filled,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_geometryBuilder || sides < 3) {
            return;
        }

        auto geo = m_geometryBuilder->CreateRegularPolygon(center, radius, sides, rotation);
        if (geo) {
            SetBrushColor(color);
            if (filled) {
                m_renderTarget->FillGeometry(geo.Get(), m_brush);
            }
            else {
                m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
            }
        }
    }

    // generate vertices then draw as a polygon
    void PrimitiveRenderer::DrawStar(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points,
        const Color& color,
        bool filled
    ) {
        if (points < 2 || !m_geometryBuilder) {
            return;
        }

        auto vertices = m_geometryBuilder->GenerateStarVertices(
            center, outerRadius, innerRadius, points
        );
        DrawPolygon(vertices, color, filled);
    }

    // draw grid by drawing individual lines
    void PrimitiveRenderer::DrawGrid(
        const Rect& bounds,
        int rows,
        int cols,
        const Color& color,
        float strokeWidth
    ) {
        if (rows <= 0 || cols <= 0) {
            return;
        }

        float dx = bounds.width / static_cast<float>(cols);
        float dy = bounds.height / static_cast<float>(rows);

        for (int i = 1; i < cols; ++i) {
            DrawLine(
                { bounds.x + i * dx, bounds.y },
                { bounds.x + i * dx, bounds.GetBottom() },
                color,
                strokeWidth
            );
        }

        for (int i = 1; i < rows; ++i) {
            DrawLine(
                { bounds.x, bounds.y + i * dy },
                { bounds.GetRight(), bounds.y + i * dy },
                color,
                strokeWidth
            );
        }
    }

    // loop and draw multiple circles
    // this is faster than creating a geometry for many small items
    void PrimitiveRenderer::DrawCircleBatch(
        const std::vector<Point>& centers,
        float radius,
        const Color& color,
        bool filled
    ) {
        if (!m_brush || !m_renderTarget) {
            return;
        }

        SetBrushColor(color);

        for (const auto& center : centers) {
            D2D1_ELLIPSE ellipse = {};
            ellipse.point = ToD2DPoint(center);
            ellipse.radiusX = radius;
            ellipse.radiusY = radius;

            if (filled) {
                m_renderTarget->FillEllipse(&ellipse, m_brush);
            }
            else {
                m_renderTarget->DrawEllipse(&ellipse, m_brush);
            }
        }
    }

    void PrimitiveRenderer::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Color& color,
        bool filled
    ) {
        if (!m_brush || !m_renderTarget) {
            return;
        }

        SetBrushColor(color);

        for (const auto& rect : rects) {
            D2D1_RECT_F d2dRect = ToD2DRect(rect);
            if (filled) {
                m_renderTarget->FillRectangle(&d2dRect, m_brush);
            }
            else {
                m_renderTarget->DrawRectangle(&d2dRect, m_brush);
            }
        }
    }

    void PrimitiveRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;
    }

} // namespace Spectrum