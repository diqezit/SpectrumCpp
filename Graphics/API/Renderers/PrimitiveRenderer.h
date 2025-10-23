#ifndef SPECTRUM_CPP_PRIMITIVE_RENDERER_H
#define SPECTRUM_CPP_PRIMITIVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PrimitiveRenderer class, which provides a comprehensive set
// of methods for drawing basic 2D shapes with Direct2D. It serves as the
// foundational drawing component for all visualizers.
//
// Key features:
// - Paint-first API for all drawing operations
// - Basic shape rendering (rectangles, circles, lines, arcs)
// - Complex shapes (polygons, stars, sectors, rings)
// - Batch rendering for performance optimization
// - Grid and pattern drawing utilities
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Delegates geometry creation to GeometryBuilder
// - Uses ComPtr for safe resource lifetime management
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"
#include <vector>

namespace Spectrum {

    class GeometryBuilder;
    class ResourceCache;
    class Paint;

    class PrimitiveRenderer final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit PrimitiveRenderer(
            GeometryBuilder* geometryBuilder,
            ResourceCache* resourceCache
        );

        PrimitiveRenderer(const PrimitiveRenderer&) = delete;
        PrimitiveRenderer& operator=(const PrimitiveRenderer&) = delete;

        void OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget) override;
        void OnDeviceLost() override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Paint-based API
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawRectangle(const Rect& rect, const Paint& paint) const;
        void DrawRoundedRectangle(const Rect& rect, float radius, const Paint& paint) const;
        void DrawCircle(const Point& center, float radius, const Paint& paint) const;
        void DrawEllipse(const Point& center, float radiusX, float radiusY, const Paint& paint) const;
        void DrawLine(const Point& start, const Point& end, const Paint& paint) const;
        void DrawPolyline(const std::vector<Point>& points, const Paint& paint) const;
        void DrawPolygon(const std::vector<Point>& points, const Paint& paint) const;
        void DrawArc(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const;
        void DrawRing(const Point& center, float innerRadius, float outerRadius, const Paint& paint) const;
        void DrawSector(const Point& center, float radius, float startAngle, float sweepAngle, const Paint& paint) const;
        void DrawRegularPolygon(const Point& center, float radius, int sides, float rotation, const Paint& paint) const;
        void DrawStar(const Point& center, float outerRadius, float innerRadius, int points, const Paint& paint) const;
        void DrawGrid(const Rect& bounds, int rows, int cols, const Paint& paint) const;
        void DrawCircleBatch(const std::vector<Point>& centers, float radius, const Paint& paint) const;
        void DrawRectangleBatch(const std::vector<Rect>& rects, const Paint& paint) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Brush & Resource Helpers (DRY)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1Brush> GetPaintBrush(const Paint& paint) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Stroke Style Management (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ApplyPaintToStrokeStyle(
            const Paint& paint,
            wrl::ComPtr<ID2D1StrokeStyle>& strokeStyle
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing Primitives (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void FillShape(ID2D1Brush* brush, const D2D1_RECT_F& rect) const;
        void FillShape(ID2D1Brush* brush, const D2D1_ROUNDED_RECT& rect) const;
        void FillShape(ID2D1Brush* brush, const D2D1_ELLIPSE& ellipse) const;
        void FillShape(ID2D1Brush* brush, ID2D1Geometry* geometry) const;

        void StrokeShape(
            ID2D1Brush* brush,
            const Paint& paint,
            const D2D1_RECT_F& rect
        ) const;

        void StrokeShape(
            ID2D1Brush* brush,
            const Paint& paint,
            const D2D1_ROUNDED_RECT& rect
        ) const;

        void StrokeShape(
            ID2D1Brush* brush,
            const Paint& paint,
            const D2D1_ELLIPSE& ellipse
        ) const;

        void StrokeShape(
            ID2D1Brush* brush,
            const Paint& paint,
            ID2D1Geometry* geometry
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Drawing (DRY)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawGeometryWithPaint(
            ID2D1Geometry* geometry,
            const Paint& paint
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;
        GeometryBuilder* m_geometryBuilder;
        ResourceCache* m_resourceCache;
        mutable Helpers::Rendering::RenderResourceCache<uint64_t, ID2D1StrokeStyle> m_strokeStyleCache;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PRIMITIVE_RENDERER_H