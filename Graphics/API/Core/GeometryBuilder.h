#ifndef SPECTRUM_CPP_GEOMETRY_BUILDER_H
#define SPECTRUM_CPP_GEOMETRY_BUILDER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the GeometryBuilder class for creating Direct2D path geometries.
//
// This class provides factory methods for complex shapes and path generation,
// serving as a centralized geometry creation service. It includes both
// instance methods (requiring ID2D1Factory) and static utility methods
// for vertex generation.
//
// Key responsibilities:
// - Path geometry creation from point arrays
// - Complex shape generation (arcs, polygons, angular slices)
// - Vertex generation for common patterns (circles, stars, waveforms)
//
// Design notes:
// - All creation methods are const (stateless operations)
// - Static methods for vertex generation (no D2D dependency)
// - Non-owning pointer to ID2D1Factory (lifetime managed externally)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <vector>

namespace Spectrum {

    class GeometryBuilder final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit GeometryBuilder(ID2D1Factory* factory);

        GeometryBuilder(const GeometryBuilder&) = delete;
        GeometryBuilder& operator=(const GeometryBuilder&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Path Geometry Creation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreatePathFromPoints(
            const std::vector<Point>& points,
            bool closed,
            bool filled
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateAngularSlice(
            const Point& center,
            float radius,
            float startAngle,
            float endAngle
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Vertex Generation (Static Utility Methods)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] static std::vector<Point> GenerateCirclePoints(
            const Point& center,
            float radius,
            int segments
        );

        [[nodiscard]] static std::vector<Point> GenerateStarVertices(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points
        );

        [[nodiscard]] static std::vector<Point> GenerateRegularPolygonVertices(
            const Point& center,
            float radius,
            int sides,
            float rotation
        );

        [[nodiscard]] static std::vector<Point> GenerateWaveformPoints(
            const SpectrumData& spectrum,
            const Rect& bounds
        );

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1Factory* m_factory;
    };

} // namespace Spectrum

#endif