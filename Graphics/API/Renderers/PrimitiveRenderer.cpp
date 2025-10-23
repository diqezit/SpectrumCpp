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

#include "Graphics/API/Renderers/PrimitiveRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"
#include "Graphics/API/Core/GeometryBuilder.h"
#include "Graphics/API/Core/ResourceCache.h"
#include "Graphics/API/Structs/Paint.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;
    using namespace Helpers::EnumConversion;
    using namespace Helpers::Rendering;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    PrimitiveRenderer::PrimitiveRenderer(
        GeometryBuilder* geometryBuilder,
        Spectrum::ResourceCache* resourceCache
    )
        : m_geometryBuilder(geometryBuilder)
        , m_resourceCache(resourceCache)
    {
    }

    void PrimitiveRenderer::OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget)
    {
        m_renderTarget = renderTarget;
        m_strokeStyleCache.Clear();
    }

    void PrimitiveRenderer::OnDeviceLost()
    {
        m_renderTarget.Reset();
        m_strokeStyleCache.Clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Brush & Resource Helpers (DRY)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1Brush> PrimitiveRenderer::GetPaintBrush(const Paint& paint) const
    {
        if (!m_resourceCache) {
            return nullptr;
        }
        return m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Stroke Style Management (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::ApplyPaintToStrokeStyle(
        const Paint& paint,
        wrl::ComPtr<ID2D1StrokeStyle>& strokeStyle
    ) const {
        if (!paint.NeedsStrokeStyle()) {
            strokeStyle.Reset();
            return;
        }

        const uint64_t key = HashGenerator::GenerateStrokeStyleKey(
            ToD2DCapStyle(paint.GetStrokeCap()),
            ToD2DCapStyle(paint.GetStrokeCap()),
            ToD2DCapStyle(paint.GetStrokeCap()),
            ToD2DLineJoin(paint.GetStrokeJoin()),
            ToD2DDashStyle(paint.GetDashStyle()),
            paint.GetDashOffset()
        );

        strokeStyle = m_strokeStyleCache.GetOrCreate(key, [&]() {
            auto factory = FactoryHelper::GetFactoryFromRenderTarget(m_renderTarget.Get());
            if (!factory) return wrl::ComPtr<ID2D1StrokeStyle>();

            auto props = StrokeStyleManager::CreateStrokeProperties(
                ToD2DCapStyle(paint.GetStrokeCap()),
                ToD2DCapStyle(paint.GetStrokeCap()),
                ToD2DCapStyle(paint.GetStrokeCap()),
                ToD2DLineJoin(paint.GetStrokeJoin()),
                paint.GetMiterLimit(),
                ToD2DDashStyle(paint.GetDashStyle()),
                paint.GetDashOffset()
            );

            return StrokeStyleManager::CreateStrokeStyle(
                factory.Get(),
                props,
                paint.GetDashPattern().data(),
                static_cast<UINT32>(paint.GetDashPattern().size())
            );
            });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Primitives (SRP) - Fill
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::FillShape(
        ID2D1Brush* brush,
        const D2D1_RECT_F& rect
    ) const {
        if (m_renderTarget && brush) {
            m_renderTarget->FillRectangle(&rect, brush);
        }
    }

    void PrimitiveRenderer::FillShape(
        ID2D1Brush* brush,
        const D2D1_ROUNDED_RECT& rect
    ) const {
        if (m_renderTarget && brush) {
            m_renderTarget->FillRoundedRectangle(&rect, brush);
        }
    }

    void PrimitiveRenderer::FillShape(
        ID2D1Brush* brush,
        const D2D1_ELLIPSE& ellipse
    ) const {
        if (m_renderTarget && brush) {
            m_renderTarget->FillEllipse(&ellipse, brush);
        }
    }

    void PrimitiveRenderer::FillShape(
        ID2D1Brush* brush,
        ID2D1Geometry* geometry
    ) const {
        if (m_renderTarget && brush && geometry) {
            m_renderTarget->FillGeometry(geometry, brush);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Primitives (SRP) - Stroke
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::StrokeShape(
        ID2D1Brush* brush,
        const Paint& paint,
        const D2D1_RECT_F& rect
    ) const {
        if (!m_renderTarget || !brush) return;

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawRectangle(
            &rect,
            brush,
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    void PrimitiveRenderer::StrokeShape(
        ID2D1Brush* brush,
        const Paint& paint,
        const D2D1_ROUNDED_RECT& rect
    ) const {
        if (!m_renderTarget || !brush) return;

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawRoundedRectangle(
            &rect,
            brush,
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    void PrimitiveRenderer::StrokeShape(
        ID2D1Brush* brush,
        const Paint& paint,
        const D2D1_ELLIPSE& ellipse
    ) const {
        if (!m_renderTarget || !brush) return;

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawEllipse(
            &ellipse,
            brush,
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    void PrimitiveRenderer::StrokeShape(
        ID2D1Brush* brush,
        const Paint& paint,
        ID2D1Geometry* geometry
    ) const {
        if (!m_renderTarget || !brush || !geometry) return;

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawGeometry(
            geometry,
            brush,
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Drawing (DRY)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawGeometryWithPaint(
        ID2D1Geometry* geometry,
        const Paint& paint
    ) const {
        if (!geometry) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        if (paint.IsFilled()) {
            FillShape(brush.Get(), geometry);
        }

        if (paint.IsStroked()) {
            StrokeShape(brush.Get(), paint, geometry);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paint-based API Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawRectangle(
        const Rect& rect,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        const D2D1_RECT_F d2dRect = ToD2DRect(rect);

        if (paint.IsFilled()) {
            FillShape(brush.Get(), d2dRect);
        }

        if (paint.IsStroked()) {
            StrokeShape(brush.Get(), paint, d2dRect);
        }
    }

    void PrimitiveRenderer::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        const float sanitizedRadius = NonNegativeFloat(radius);
        const D2D1_ROUNDED_RECT rr = {
            ToD2DRect(rect),
            sanitizedRadius,
            sanitizedRadius
        };

        if (paint.IsFilled()) {
            FillShape(brush.Get(), rr);
        }

        if (paint.IsStroked()) {
            StrokeShape(brush.Get(), paint, rr);
        }
    }

    void PrimitiveRenderer::DrawCircle(
        const Point& center,
        float radius,
        const Paint& paint
    ) const {
        DrawEllipse(center, radius, radius, paint);
    }

    void PrimitiveRenderer::DrawEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!PositiveRadius(radiusX) || !PositiveRadius(radiusY)) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        const D2D1_ELLIPSE ellipse = ToD2DEllipse(center, radiusX, radiusY);

        if (paint.IsFilled()) {
            FillShape(brush.Get(), ellipse);
        }

        if (paint.IsStroked()) {
            StrokeShape(brush.Get(), paint, ellipse);
        }
    }

    void PrimitiveRenderer::DrawLine(
        const Point& start,
        const Point& end,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawLine(
            ToD2DPoint(start),
            ToD2DPoint(end),
            brush.Get(),
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    void PrimitiveRenderer::DrawPolyline(
        const std::vector<Point>& points,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!m_geometryBuilder) return;
        if (!PointArray(points, 2)) return;

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);
        if (!geo) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        StrokeShape(brush.Get(), paint, geo.Get());
    }

    void PrimitiveRenderer::DrawPolygon(
        const std::vector<Point>& points,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!m_geometryBuilder) return;
        if (!PointArray(points, 3)) return;

        auto geo = m_geometryBuilder->CreatePathFromPoints(
            points,
            true,
            paint.IsFilled()
        );

        DrawGeometryWithPaint(geo.Get(), paint);
    }

    void PrimitiveRenderer::DrawArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!m_geometryBuilder) return;
        if (!PositiveRadius(radius) || !NonZeroAngle(sweepAngle)) return;

        auto geo = m_geometryBuilder->CreateArc(center, radius, startAngle, sweepAngle);
        if (!geo) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        StrokeShape(brush.Get(), paint, geo.Get());
    }

    void PrimitiveRenderer::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Paint& paint
    ) const {
        if (!RadiusRange(innerRadius, outerRadius)) return;

        const float strokeWidth = outerRadius - innerRadius;
        const float radius = innerRadius + strokeWidth * 0.5f;

        DrawCircle(
            center,
            radius,
            paint.WithStyle(PaintStyle::Stroke).WithStrokeWidth(strokeWidth)
        );
    }

    void PrimitiveRenderer::DrawSector(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!m_geometryBuilder) return;
        if (!PositiveRadius(radius) || !NonZeroAngle(sweepAngle)) return;

        auto geo = m_geometryBuilder->CreateAngularSlice(
            center,
            radius,
            startAngle,
            startAngle + sweepAngle
        );

        DrawGeometryWithPaint(geo.Get(), paint);
    }

    void PrimitiveRenderer::DrawRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation,
        const Paint& paint
    ) const {
        if (!m_geometryBuilder) return;

        const auto vertices = m_geometryBuilder->GenerateRegularPolygonVertices(
            center,
            radius,
            PolygonSides(sides),
            rotation
        );

        DrawPolygon(vertices, paint);
    }

    void PrimitiveRenderer::DrawStar(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points,
        const Paint& paint
    ) const {
        if (!m_geometryBuilder) return;
        if (!RadiusRange(innerRadius, outerRadius)) return;

        const auto vertices = m_geometryBuilder->GenerateStarVertices(
            center,
            outerRadius,
            innerRadius,
            StarPoints(points)
        );

        DrawPolygon(vertices, paint);
    }

    void PrimitiveRenderer::DrawGrid(
        const Rect& bounds,
        int rows,
        int cols,
        const Paint& paint
    ) const {
        const int sanitizedRows = MinValue(rows, 1);
        const int sanitizedCols = MinValue(cols, 1);
        const float dx = bounds.width / static_cast<float>(sanitizedCols);
        const float dy = bounds.height / static_cast<float>(sanitizedRows);

        for (int i = 1; i < sanitizedCols; ++i) {
            const float x = bounds.x + i * dx;
            DrawLine({ x, bounds.y }, { x, bounds.GetBottom() }, paint);
        }

        for (int i = 1; i < sanitizedRows; ++i) {
            const float y = bounds.y + i * dy;
            DrawLine({ bounds.x, y }, { bounds.GetRight(), y }, paint);
        }
    }

    void PrimitiveRenderer::DrawCircleBatch(
        const std::vector<Point>& centers,
        float radius,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!PositiveRadius(radius)) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        for (const auto& center : centers) {
            const D2D1_ELLIPSE ellipse = ToD2DEllipse(center, radius, radius);
            if (paint.IsFilled()) {
                FillShape(brush.Get(), ellipse);
            }
        }
    }

    void PrimitiveRenderer::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Paint& paint
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;

        auto brush = GetPaintBrush(paint);
        if (!brush) return;

        for (const auto& rect : rects) {
            const D2D1_RECT_F d2dRect = ToD2DRect(rect);
            if (paint.IsFilled()) {
                FillShape(brush.Get(), d2dRect);
            }
        }
    }

} // namespace Spectrum