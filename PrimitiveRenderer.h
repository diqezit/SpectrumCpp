// PrimitiveRenderer.h
#ifndef SPECTRUM_CPP_PRIMITIVE_RENDERER_H
#define SPECTRUM_CPP_PRIMITIVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PrimitiveRenderer class, which provides a comprehensive set
// of methods for drawing basic 2D shapes with Direct2D. It serves as the
// foundational drawing component for all visualizers.
//
// Key responsibilities:
// - Basic shape rendering (rectangles, circles, lines, arcs)
// - Complex shapes (polygons, stars, sectors, rings)
// - Batch rendering for performance optimization
// - Grid and pattern drawing utilities
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Delegates geometry creation to GeometryBuilder
// - Supports both filled and stroked rendering modes
// - Non-owning pointers to dependencies (lifetime managed externally)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "GeometryBuilder.h"
#include <vector>

namespace Spectrum {

    class PrimitiveRenderer final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        PrimitiveRenderer(
            ID2D1RenderTarget* renderTarget,
            ID2D1SolidColorBrush* brush,
            GeometryBuilder* geometryBuilder
        );

        PrimitiveRenderer(const PrimitiveRenderer&) = delete;
        PrimitiveRenderer& operator=(const PrimitiveRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Basic Shape Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawRectangle(
            const Rect& rect,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawRoundedRectangle(
            const Rect& rect,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawCircle(
            const Point& center,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawEllipse(
            const Point& center,
            float radiusX,
            float radiusY,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawLine(
            const Point& start,
            const Point& end,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Path and Polygon Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawPolyline(
            const std::vector<Point>& points,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        void DrawPolygon(
            const std::vector<Point>& points,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Arc and Sector Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        void DrawRing(
            const Point& center,
            float innerRadius,
            float outerRadius,
            const Color& color
        ) const;

        void DrawSector(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            bool filled = true
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Complex Shape Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        ) const;

        void DrawStar(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points,
            const Color& color,
            bool filled = true
        ) const;

        void DrawGrid(
            const Rect& bounds,
            int rows,
            int cols,
            const Color& color,
            float strokeWidth = 1.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Batch Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawCircleBatch(
            const std::vector<Point>& centers,
            float radius,
            const Color& color,
            bool filled = true
        ) const;

        void DrawRectangleBatch(
            const std::vector<Rect>& rects,
            const Color& color,
            bool filled = true
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetBrushColor(const Color& color) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1RenderTarget* m_renderTarget;
        ID2D1SolidColorBrush* m_brush;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif