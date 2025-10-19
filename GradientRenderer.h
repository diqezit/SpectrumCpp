#ifndef SPECTRUM_CPP_GRADIENT_RENDERER_H
#define SPECTRUM_CPP_GRADIENT_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the GradientRenderer class, which is responsible
// for drawing shapes filled with various types of gradients. It uses
// ResourceCache to avoid recreating expensive gradient brushes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "ResourceCache.h"
#include "GeometryBuilder.h"
#include <vector>

namespace Spectrum {

    class GradientRenderer {
    public:
        GradientRenderer(
            ID2D1RenderTarget* renderTarget,
            ID2D1SolidColorBrush* solidBrush,
            ResourceCache* cache,
            GeometryBuilder* geometryBuilder
        );

        void DrawGradientRectangle(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool horizontal = true
        );

        void DrawRadialGradient(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        );

        void DrawGradientCircle(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool filled = true
        );

        void DrawGradientPath(
            const std::vector<Point>& points,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float strokeWidth = 1.0f
        );

        void DrawAngularGradient(
            const Point& center,
            float radius,
            float startAngle,
            float endAngle,
            const Color& startColor,
            const Color& endColor
        );

        void DrawVerticalGradientBar(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float cornerRadius = 0.0f
        );

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        ID2D1RenderTarget* m_renderTarget;
        ID2D1SolidColorBrush* m_solidBrush;
        ResourceCache* m_cache;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif