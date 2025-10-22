// PrimitiveRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the PrimitiveRenderer class. This file wraps Direct2D drawing
// functions to provide a simplified, consistent API for drawing shapes.
//
// Implementation details:
// - All methods validate input parameters before rendering
// - Geometry creation delegated to GeometryBuilder
// - Batch methods optimize draw calls for collections
// - Uses D2DHelpers for validation, sanitization, and conversion
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "PrimitiveRenderer.h"
#include "D2DHelpers.h"
#include "Core/GeometryBuilder.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    PrimitiveRenderer::PrimitiveRenderer(GeometryBuilder* geometryBuilder)
        : m_renderTarget(nullptr)
        , m_brush(nullptr)
        , m_geometryBuilder(geometryBuilder)
    {
    }

    void PrimitiveRenderer::OnRenderTargetChanged(ID2D1RenderTarget* renderTarget)
    {
        m_renderTarget = renderTarget;
    }

    void PrimitiveRenderer::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        m_brush = nullptr;
    }

    void PrimitiveRenderer::SetSolidBrush(ID2D1SolidColorBrush* brush)
    {
        m_brush = brush;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Basic Shape Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawRectangle(
        const Rect& rect,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;

        SetBrushColor(color);
        const D2D1_RECT_F d2dRect = ToD2DRect(rect);

        if (filled) m_renderTarget->FillRectangle(&d2dRect, m_brush);
        else m_renderTarget->DrawRectangle(&d2dRect, m_brush, strokeWidth);
    }

    void PrimitiveRenderer::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;

        SetBrushColor(color);
        const float sanitizedRadius = NonNegativeFloat(radius);
        const D2D1_ROUNDED_RECT rr = { ToD2DRect(rect), sanitizedRadius, sanitizedRadius };

        if (filled) m_renderTarget->FillRoundedRectangle(&rr, m_brush);
        else m_renderTarget->DrawRoundedRectangle(&rr, m_brush, strokeWidth);
    }

    void PrimitiveRenderer::DrawCircle(
        const Point& center,
        float radius,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        DrawEllipse(center, radius, radius, color, filled, strokeWidth);
    }

    void PrimitiveRenderer::DrawEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!PositiveRadius(radiusX) || !PositiveRadius(radiusY)) return;

        SetBrushColor(color);
        const D2D1_ELLIPSE ellipse = {
            ToD2DPoint(center),
            radiusX,
            radiusY
        };

        if (filled) m_renderTarget->FillEllipse(&ellipse, m_brush);
        else m_renderTarget->DrawEllipse(&ellipse, m_brush, strokeWidth);
    }

    void PrimitiveRenderer::DrawLine(
        const Point& start,
        const Point& end,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;

        SetBrushColor(color);
        m_renderTarget->DrawLine(
            ToD2DPoint(start),
            ToD2DPoint(end),
            m_brush,
            strokeWidth
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Path and Polygon Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawPolyline(
        const std::vector<Point>& points,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!m_geometryBuilder) return;
        if (!PointArray(points, 2)) return;

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);
        if (!geo) return;

        SetBrushColor(color);
        m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
    }

    void PrimitiveRenderer::DrawPolygon(
        const std::vector<Point>& points,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!m_geometryBuilder) return;
        if (!PointArray(points, 3)) return;

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, true, filled);
        if (!geo) return;

        SetBrushColor(color);

        if (filled) m_renderTarget->FillGeometry(geo.Get(), m_brush);
        else m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Arc and Sector Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Color& color,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!m_geometryBuilder) return;
        if (!PositiveRadius(radius)) return;
        if (!NonZeroAngle(sweepAngle)) return;

        auto geo = m_geometryBuilder->CreateArc(center, radius, startAngle, sweepAngle);
        if (!geo) return;

        SetBrushColor(color);
        m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
    }

    void PrimitiveRenderer::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Color& color
    ) const
    {
        if (!m_renderTarget) return;
        if (!RadiusRange(innerRadius, outerRadius)) return;

        const float strokeWidth = outerRadius - innerRadius;
        const float radius = innerRadius + strokeWidth * 0.5f;

        DrawCircle(center, radius, color, false, strokeWidth);
    }

    void PrimitiveRenderer::DrawSector(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Color& color,
        bool filled
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!m_geometryBuilder) return;
        if (!PositiveRadius(radius)) return;
        if (!NonZeroAngle(sweepAngle)) return;

        auto geo = m_geometryBuilder->CreateAngularSlice(
            center, radius, startAngle, startAngle + sweepAngle
        );
        if (!geo) return;

        SetBrushColor(color);

        if (filled) m_renderTarget->FillGeometry(geo.Get(), m_brush);
        else m_renderTarget->DrawGeometry(geo.Get(), m_brush, 1.0f);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Complex Shape Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation,
        const Color& color,
        bool filled,
        float strokeWidth
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!m_geometryBuilder) return;
        if (!PositiveRadius(radius)) return;

        const int sanitizedSides = PolygonSides(sides);

        auto geo = m_geometryBuilder->CreateRegularPolygon(center, radius, sanitizedSides, rotation);
        if (!geo) return;

        SetBrushColor(color);

        if (filled) m_renderTarget->FillGeometry(geo.Get(), m_brush);
        else m_renderTarget->DrawGeometry(geo.Get(), m_brush, strokeWidth);
    }

    void PrimitiveRenderer::DrawStar(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points,
        const Color& color,
        bool filled
    ) const
    {
        if (!m_geometryBuilder) return;
        if (!RadiusRange(innerRadius, outerRadius)) return;

        const int sanitizedPoints = StarPoints(points);

        auto vertices = m_geometryBuilder->GenerateStarVertices(
            center, outerRadius, innerRadius, sanitizedPoints
        );

        DrawPolygon(vertices, color, filled);
    }

    void PrimitiveRenderer::DrawGrid(
        const Rect& bounds,
        int rows,
        int cols,
        const Color& color,
        float strokeWidth
    ) const
    {
        const int sanitizedRows = MinValue(rows, 1);
        const int sanitizedCols = MinValue(cols, 1);

        const float dx = bounds.width / static_cast<float>(sanitizedCols);
        const float dy = bounds.height / static_cast<float>(sanitizedRows);

        for (int i = 1; i < sanitizedCols; ++i) {
            const float x = bounds.x + i * dx;
            DrawLine(
                { x, bounds.y },
                { x, bounds.GetBottom() },
                color,
                strokeWidth
            );
        }

        for (int i = 1; i < sanitizedRows; ++i) {
            const float y = bounds.y + i * dy;
            DrawLine(
                { bounds.x, y },
                { bounds.GetRight(), y },
                color,
                strokeWidth
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Batch Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawCircleBatch(
        const std::vector<Point>& centers,
        float radius,
        const Color& color,
        bool filled
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!PositiveRadius(radius)) return;

        SetBrushColor(color);

        for (const auto& center : centers) {
            const D2D1_ELLIPSE ellipse = {
                ToD2DPoint(center),
                radius,
                radius
            };

            if (filled) m_renderTarget->FillEllipse(&ellipse, m_brush);
            else m_renderTarget->DrawEllipse(&ellipse, m_brush);
        }
    }

    void PrimitiveRenderer::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Color& color,
        bool filled
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;

        SetBrushColor(color);

        for (const auto& rect : rects) {
            const D2D1_RECT_F d2dRect = ToD2DRect(rect);

            if (filled) m_renderTarget->FillRectangle(&d2dRect, m_brush);
            else m_renderTarget->DrawRectangle(&d2dRect, m_brush);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::SetBrushColor(const Color& color) const
    {
        if (!m_brush) return;
        m_brush->SetColor(ToD2DColor(color));
    }

} // namespace Spectrum