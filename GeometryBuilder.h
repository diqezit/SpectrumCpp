#ifndef SPECTRUM_CPP_GEOMETRY_BUILDER_H
#define SPECTRUM_CPP_GEOMETRY_BUILDER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the GeometryBuilder class, which is responsible for
// creating D2D1PathGeometry objects for complex shapes and generating
// vertex data for primitive renderers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <vector>

namespace Spectrum {

    class GeometryBuilder {
    public:
        explicit GeometryBuilder(ID2D1Factory* factory);

        wrl::ComPtr<ID2D1PathGeometry> CreatePathFromPoints(
            const std::vector<Point>& points,
            bool closed,
            bool filled
        );

        wrl::ComPtr<ID2D1PathGeometry> CreateArc(
            const Point& center,
            float radius,
            float startAngle,
            float sweepAngle
        );

        wrl::ComPtr<ID2D1PathGeometry> CreateRegularPolygon(
            const Point& center,
            float radius,
            int sides,
            float rotation
        );

        wrl::ComPtr<ID2D1PathGeometry> CreateAngularSlice(
            const Point& center,
            float radius,
            float startAngle,
            float endAngle
        );

        std::vector<Point> GenerateCirclePoints(
            const Point& center,
            float radius,
            int segments
        );

        std::vector<Point> GenerateStarVertices(
            const Point& center,
            float outerRadius,
            float innerRadius,
            int points
        );

        std::vector<Point> GenerateWaveformPoints(
            const SpectrumData& spectrum,
            const Rect& bounds
        );

    private:
        ID2D1Factory* m_factory;
    };

} // namespace Spectrum

#endif