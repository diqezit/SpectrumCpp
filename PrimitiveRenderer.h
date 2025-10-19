#ifndef SPECTRUM_CPP_PRIMITIVE_RENDERER_H
#define SPECTRUM_CPP_PRIMITIVE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the PrimitiveRenderer class, which provides a
// comprehensive set of methods for drawing basic 2D shapes. It serves
// as the foundational drawing component for all visualizers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "GeometryBuilder.h"
#include <vector>

namespace Spectrum {

    class PrimitiveRenderer {
    public:
        PrimitiveRenderer(
            ID2D1RenderTarget* renderTarget,
            ID2D1SolidColorBrush* brush,
            GeometryBuilder* geometryBuilder
        );

        void DrawRectangle(
            const Rect& rect,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawRoundedRectangle(
            const Rect& rect,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawCircle(
            const Point& center,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawEllipse(
            const Point& center,
            float radiusX,
            float radiusY,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawLine(
            const Point& start,
            const Point& end,
            const Color& color,
            float strokeWidth = 1.0f
        );

        void DrawPolyline(
            const std::vector<Point>& points,
            const Color& color,
            float strokeWidth = 1.0f
        );

        void DrawPolygon(
            const std::vector<Point>& points,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            float strokeWidth = 1.0f
        );

        void DrawRing(
            const Point& center,
            float innerRadius,
            float outerRadius,
            const Color& color
        );

        void DrawSector(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle,
            const Color& color,
            bool filled = true
        );

        void DrawRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );

        void DrawStar(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points,
            const Color& color,
            bool filled = true
        );

        void DrawGrid(
            const Rect& bounds,
            int rows,
            int cols,
            const Color& color,
            float strokeWidth = 1.0f
        );

        // Batch rendering for performance
        void DrawCircleBatch(
            const std::vector<Point>& centers,
            float radius,
            const Color& color,
            bool filled = true
        );

        void DrawRectangleBatch(
            const std::vector<Rect>& rects,
            const Color& color,
            bool filled = true
        );

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        void SetBrushColor(const Color& color);

        ID2D1RenderTarget* m_renderTarget;
        ID2D1SolidColorBrush* m_brush;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif