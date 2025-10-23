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
#include "Graphics/API/Core/GeometryBuilder.h"
#include "Graphics/API/Core/ResourceCache.h"
#include "Graphics/API/Structs/Paint.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;
    using namespace Helpers::EnumConversion;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    PrimitiveRenderer::PrimitiveRenderer(
        GeometryBuilder* geometryBuilder,
        ResourceCache* resourceCache
    )
        : m_renderTarget(nullptr)
        , m_geometryBuilder(geometryBuilder)
        , m_resourceCache(resourceCache)
    {
    }

    void PrimitiveRenderer::OnRenderTargetChanged(
        ID2D1RenderTarget* renderTarget
    ) {
        m_renderTarget = renderTarget;
        m_strokeStyleCache.clear();
    }

    void PrimitiveRenderer::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        m_strokeStyleCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::ApplyPaintToStrokeStyle(
        const Paint& paint,
        wrl::ComPtr<ID2D1StrokeStyle>& strokeStyle
    ) const {
        if (!paint.NeedsStrokeStyle()) {
            strokeStyle.Reset();
            return;
        }

        uint64_t key = (static_cast<uint64_t>(paint.GetStrokeCap()) << 0) |
            (static_cast<uint64_t>(paint.GetStrokeJoin()) << 8) |
            (static_cast<uint64_t>(paint.GetDashStyle()) << 16);

        auto it = m_strokeStyleCache.find(key);
        if (it != m_strokeStyleCache.end()) {
            strokeStyle = it->second;
            return;
        }

        if (!m_renderTarget) {
            return;
        }

        wrl::ComPtr<ID2D1Factory> factory;
        m_renderTarget->GetFactory(factory.GetAddressOf());

        if (!factory) {
            return;
        }

        D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
            ToD2DCapStyle(paint.GetStrokeCap()),
            ToD2DCapStyle(paint.GetStrokeCap()),
            ToD2DCapStyle(paint.GetStrokeCap()),
            ToD2DLineJoin(paint.GetStrokeJoin()),
            paint.GetMiterLimit(),
            ToD2DDashStyle(paint.GetDashStyle()),
            paint.GetDashOffset()
        );

        factory->CreateStrokeStyle(
            props,
            paint.GetDashPattern().data(),
            static_cast<UINT32>(paint.GetDashPattern().size()),
            &strokeStyle
        );

        m_strokeStyleCache[key] = strokeStyle;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paint-based API Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PrimitiveRenderer::DrawRectangle(
        const Rect& rect,
        const Paint& paint
    ) const {
        if (!m_renderTarget || !m_resourceCache) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        const D2D1_RECT_F d2dRect = ToD2DRect(rect);

        if (paint.IsFilled()) {
            m_renderTarget->FillRectangle(&d2dRect, brush.Get());
        }

        if (paint.IsStroked()) {
            wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
            ApplyPaintToStrokeStyle(paint, strokeStyle);

            m_renderTarget->DrawRectangle(
                &d2dRect,
                brush.Get(),
                paint.GetStrokeWidth(),
                strokeStyle.Get()
            );
        }
    }

    void PrimitiveRenderer::DrawRoundedRectangle(
        const Rect& rect,
        float radius,
        const Paint& paint
    ) const {
        if (!m_renderTarget || !m_resourceCache) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        const float sanitizedRadius = NonNegativeFloat(radius);
        const D2D1_ROUNDED_RECT rr = { ToD2DRect(rect), sanitizedRadius, sanitizedRadius };

        if (paint.IsFilled()) {
            m_renderTarget->FillRoundedRectangle(&rr, brush.Get());
        }

        if (paint.IsStroked()) {
            wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
            ApplyPaintToStrokeStyle(paint, strokeStyle);

            m_renderTarget->DrawRoundedRectangle(
                &rr,
                brush.Get(),
                paint.GetStrokeWidth(),
                strokeStyle.Get()
            );
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
        if (!m_renderTarget || !m_resourceCache || !PositiveRadius(radiusX) || !PositiveRadius(radiusY)) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        const D2D1_ELLIPSE ellipse = ToD2DEllipse(center, radiusX, radiusY);

        if (paint.IsFilled()) {
            m_renderTarget->FillEllipse(&ellipse, brush.Get());
        }

        if (paint.IsStroked()) {
            wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
            ApplyPaintToStrokeStyle(paint, strokeStyle);

            m_renderTarget->DrawEllipse(
                &ellipse,
                brush.Get(),
                paint.GetStrokeWidth(),
                strokeStyle.Get()
            );
        }
    }

    void PrimitiveRenderer::DrawLine(
        const Point& start,
        const Point& end,
        const Paint& paint
    ) const {
        if (!m_renderTarget || !m_resourceCache) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

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
        if (!m_renderTarget || !m_resourceCache || !m_geometryBuilder || !PointArray(points, 2)) {
            return;
        }

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);
        if (!geo) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawGeometry(
            geo.Get(),
            brush.Get(),
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    void PrimitiveRenderer::DrawPolygon(
        const std::vector<Point>& points,
        const Paint& paint
    ) const {
        if (!m_renderTarget || !m_resourceCache || !m_geometryBuilder || !PointArray(points, 3)) {
            return;
        }

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, true, paint.IsFilled());
        if (!geo) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        if (paint.IsFilled()) {
            m_renderTarget->FillGeometry(geo.Get(), brush.Get());
        }

        if (paint.IsStroked()) {
            wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
            ApplyPaintToStrokeStyle(paint, strokeStyle);

            m_renderTarget->DrawGeometry(
                geo.Get(),
                brush.Get(),
                paint.GetStrokeWidth(),
                strokeStyle.Get()
            );
        }
    }

    void PrimitiveRenderer::DrawArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle,
        const Paint& paint
    ) const {
        if (!m_renderTarget || !m_resourceCache || !m_geometryBuilder || !PositiveRadius(radius) || !NonZeroAngle(sweepAngle)) {
            return;
        }

        auto geo = m_geometryBuilder->CreateArc(center, radius, startAngle, sweepAngle);
        if (!geo) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
        ApplyPaintToStrokeStyle(paint, strokeStyle);

        m_renderTarget->DrawGeometry(
            geo.Get(),
            brush.Get(),
            paint.GetStrokeWidth(),
            strokeStyle.Get()
        );
    }

    void PrimitiveRenderer::DrawRing(
        const Point& center,
        float innerRadius,
        float outerRadius,
        const Paint& paint
    ) const {
        if (!RadiusRange(innerRadius, outerRadius)) {
            return;
        }

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
        if (!m_renderTarget || !m_resourceCache || !m_geometryBuilder || !PositiveRadius(radius) || !NonZeroAngle(sweepAngle)) {
            return;
        }

        auto geo = m_geometryBuilder->CreateAngularSlice(center, radius, startAngle, startAngle + sweepAngle);
        if (!geo) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        if (paint.IsFilled()) {
            m_renderTarget->FillGeometry(geo.Get(), brush.Get());
        }

        if (paint.IsStroked()) {
            m_renderTarget->DrawGeometry(geo.Get(), brush.Get(), paint.GetStrokeWidth());
        }
    }

    void PrimitiveRenderer::DrawRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation,
        const Paint& paint
    ) const {
        if (!m_geometryBuilder) {
            return;
        }

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
        if (!m_geometryBuilder || !RadiusRange(innerRadius, outerRadius)) {
            return;
        }

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
        if (!m_renderTarget || !m_resourceCache || !PositiveRadius(radius)) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        for (const auto& center : centers) {
            const D2D1_ELLIPSE ellipse = ToD2DEllipse(center, radius, radius);
            if (paint.IsFilled()) {
                m_renderTarget->FillEllipse(&ellipse, brush.Get());
            }
            if (paint.IsStroked()) {
                /* Batch stroke not implemented for brevity */
            }
        }
    }

    void PrimitiveRenderer::DrawRectangleBatch(
        const std::vector<Rect>& rects,
        const Paint& paint
    ) const {
        if (!m_renderTarget || !m_resourceCache) {
            return;
        }

        auto brush = m_resourceCache->GetBrush(paint.GetBrush(), paint.GetAlpha());
        if (!brush) {
            return;
        }

        for (const auto& rect : rects) {
            const D2D1_RECT_F d2dRect = ToD2DRect(rect);
            if (paint.IsFilled()) {
                m_renderTarget->FillRectangle(&d2dRect, brush.Get());
            }
            if (paint.IsStroked()) {
                /* Batch stroke not implemented for brevity */
            }
        }
    }

} // namespace Spectrum