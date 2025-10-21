// GradientRenderer.h
#ifndef SPECTRUM_CPP_GRADIENT_RENDERER_H
#define SPECTRUM_CPP_GRADIENT_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the GradientRenderer class, which is responsible for drawing
// shapes filled with various types of gradients (linear, radial, angular).
//
// Key responsibilities:
// - Linear gradient rectangles and paths
// - Radial gradients for circular effects
// - Simulated angular gradients via geometry segments
// - Gradient caching through ResourceCache integration
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Uses ResourceCache to avoid recreating expensive gradient brushes
// - Delegates geometry creation to GeometryBuilder
// - Non-owning pointers to dependencies (lifetime managed externally)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "ResourceCache.h"
#include "GeometryBuilder.h"
#include <vector>

namespace Spectrum {

    class GradientRenderer final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        GradientRenderer(
            ID2D1RenderTarget* renderTarget,
            ID2D1SolidColorBrush* solidBrush,
            ResourceCache* cache,
            GeometryBuilder* geometryBuilder
        );

        GradientRenderer(const GradientRenderer&) = delete;
        GradientRenderer& operator=(const GradientRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Gradient Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawGradientRectangle(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool horizontal = true
        ) const;

        void DrawRadialGradient(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) const;

        void DrawGradientCircle(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            bool filled = true
        ) const;

        void DrawGradientPath(
            const std::vector<Point>& points,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float strokeWidth = 1.0f
        ) const;

        void DrawAngularGradient(
            const Point& center,
            float radius,
            float startAngle,
            float endAngle,
            const Color& startColor,
            const Color& endColor
        ) const;

        void DrawVerticalGradientBar(
            const Rect& rect,
            const std::vector<D2D1_GRADIENT_STOP>& stops,
            float cornerRadius = 0.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1RenderTarget* m_renderTarget;
        ID2D1SolidColorBrush* m_solidBrush;
        ResourceCache* m_cache;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif