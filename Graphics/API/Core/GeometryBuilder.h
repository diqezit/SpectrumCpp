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
// - Manages ID2D1Factory lifetime via ComPtr
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

        explicit GeometryBuilder(wrl::ComPtr<ID2D1Factory> factory);

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
        // Private Implementation - Geometry Creation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> CreateEmptyGeometry() const;

        [[nodiscard]] wrl::ComPtr<ID2D1GeometrySink> OpenGeometrySink(
            ID2D1PathGeometry* geometry
        ) const;

        [[nodiscard]] bool CloseGeometrySink(ID2D1GeometrySink* sink) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Path Building
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void BuildPathFromPoints(
            ID2D1GeometrySink* sink,
            const std::vector<Point>& points,
            bool closed,
            bool filled
        ) const;

        void BuildArcPath(
            ID2D1GeometrySink* sink,
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle
        ) const;

        void BuildPolygonPath(
            ID2D1GeometrySink* sink,
            const std::vector<Point>& vertices
        ) const;

        void BuildSlicePath(
            ID2D1GeometrySink* sink,
            const Point& center,
            float radius,
            float startAngle,
            float endAngle
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Arc Segment Creation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] D2D1_ARC_SEGMENT CreateArcSegment(
            const Point& endPoint,
            float radius,
            float sweepAngle
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Point Calculation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Point CalculatePointOnCircle(
            const Point& center,
            float radius,
            float angleDegrees
        ) const;

        [[nodiscard]] Point CalculatePointOnCircleRad(
            const Point& center,
            float radius,
            float angleRadians
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Vertex Generation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct RotationState {
            float cosAngle;
            float sinAngle;
            float cosStep;
            float sinStep;
        };

        [[nodiscard]] static RotationState InitializeRotation(
            float startAngle,
            float angleStep
        );

        static void ApplyRotation(RotationState& state);

        [[nodiscard]] static Point CalculatePointFromRotation(
            const Point& center,
            float radius,
            const RotationState& state
        );

        static void GeneratePointsDirect(
            std::vector<Point>& points,
            const Point& center,
            float radius,
            int count,
            float startAngle,
            float angleStep
        );

        static void GeneratePointsIncremental(
            std::vector<Point>& points,
            const Point& center,
            float radius,
            int count,
            float startAngle,
            float angleStep
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CanCreateGeometry() const noexcept { return m_factory.Get() != nullptr; }

        [[nodiscard]] bool ValidatePointArray(
            const std::vector<Point>& points,
            size_t minPoints
        ) const noexcept;

        [[nodiscard]] bool ValidateArcParameters(
            float radius,
            float sweepAngle
        ) const noexcept;

        [[nodiscard]] bool ValidateSliceParameters(
            float radius
        ) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        wrl::ComPtr<ID2D1Factory> m_factory;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GEOMETRY_BUILDER_H