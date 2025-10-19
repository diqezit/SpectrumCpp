// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the GeometryBuilder class methods
// It uses a D2D1Factory to build complex ID2D1PathGeometry objects
// and provides helper functions to generate vertex lists for shapes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GeometryBuilder.h"
#include "MathUtils.h"

namespace Spectrum {

    namespace {
        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }

        inline float DegreesToRadians(float degrees) {
            return degrees * (PI / 180.0f);
        }
    }

    GeometryBuilder::GeometryBuilder(ID2D1Factory* factory)
        : m_factory(factory)
    {
    }

    // D2D requires at least 2 points for valid path
    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreatePathFromPoints(
        const std::vector<Point>& points,
        bool closed,
        bool filled
    ) {
        if (points.size() < 2 || !m_factory) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1PathGeometry> geo;
        if (FAILED(m_factory->CreatePathGeometry(geo.GetAddressOf()))) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geo->Open(sink.GetAddressOf()))) {
            return nullptr;
        }

        sink->BeginFigure(
            ToD2DPoint(points[0]),
            filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW
        );

        for (size_t i = 1; i < points.size(); ++i) {
            sink->AddLine(ToD2DPoint(points[i]));
        }

        sink->EndFigure(
            closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN
        );
        sink->Close();

        return geo;
    }

    // D2D arc API needs explicit large/small flag when sweep >= 180 degrees
    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle
    ) {
        if (!m_factory) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1PathGeometry> geo;
        if (FAILED(m_factory->CreatePathGeometry(geo.GetAddressOf()))) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geo->Open(sink.GetAddressOf()))) {
            return nullptr;
        }

        Point startPoint = {
            center.x + radius * cosf(DegreesToRadians(startAngle)),
            center.y + radius * sinf(DegreesToRadians(startAngle))
        };

        sink->BeginFigure(ToD2DPoint(startPoint), D2D1_FIGURE_BEGIN_HOLLOW);

        D2D1_ARC_SEGMENT arc = {};
        arc.size = D2D1::SizeF(radius, radius);
        arc.rotationAngle = 0.0f;
        arc.sweepDirection = sweepAngle > 0.0f
            ? D2D1_SWEEP_DIRECTION_CLOCKWISE
            : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
        // prevent rendering wrong arc when angle crosses 180
        arc.arcSize = fabsf(sweepAngle) >= 180.0f
            ? D2D1_ARC_SIZE_LARGE
            : D2D1_ARC_SIZE_SMALL;
        arc.point = {
            center.x + radius * cosf(DegreesToRadians(startAngle + sweepAngle)),
            center.y + radius * sinf(DegreesToRadians(startAngle + sweepAngle))
        };

        sink->AddArc(&arc);
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();

        return geo;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation
    ) {
        if (!m_factory) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1PathGeometry> geo;
        if (FAILED(m_factory->CreatePathGeometry(geo.GetAddressOf()))) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geo->Open(sink.GetAddressOf()))) {
            return nullptr;
        }

        float angleStep = TWO_PI / static_cast<float>(sides);
        float rotationRad = DegreesToRadians(rotation);

        Point startPoint = {
            center.x + radius * cosf(rotationRad),
            center.y + radius * sinf(rotationRad)
        };

        sink->BeginFigure(ToD2DPoint(startPoint), D2D1_FIGURE_BEGIN_FILLED);

        for (int i = 1; i <= sides; ++i) {
            float angle = i * angleStep + rotationRad;
            sink->AddLine({
                center.x + radius * cosf(angle),
                center.y + radius * sinf(angle)
                });
        }

        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();

        return geo;
    }

    // creates triangle from center for angular gradient rendering
    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateAngularSlice(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle
    ) {
        if (!m_factory) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1PathGeometry> geo;
        if (FAILED(m_factory->CreatePathGeometry(geo.GetAddressOf()))) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geo->Open(sink.GetAddressOf()))) {
            return nullptr;
        }

        sink->BeginFigure(ToD2DPoint(center), D2D1_FIGURE_BEGIN_FILLED);

        sink->AddLine({
            center.x + radius * cosf(DegreesToRadians(startAngle)),
            center.y + radius * sinf(DegreesToRadians(startAngle))
            });

        sink->AddLine({
            center.x + radius * cosf(DegreesToRadians(endAngle)),
            center.y + radius * sinf(DegreesToRadians(endAngle))
            });

        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();

        return geo;
    }

    // reserve prevents reallocations during loop
    // extra point closes circle back to start
    std::vector<Point> GeometryBuilder::GenerateCirclePoints(
        const Point& center,
        float radius,
        int segments
    ) {
        std::vector<Point> points;
        points.reserve(static_cast<size_t>(segments) + 1);

        float angleStep = TWO_PI / static_cast<float>(segments);
        for (int i = 0; i <= segments; ++i) {
            float angle = i * angleStep;
            points.push_back({
                center.x + radius * cosf(angle),
                center.y + radius * sinf(angle)
                });
        }

        return points;
    }

    // alternates outer and inner radius to create star shape
    std::vector<Point> GeometryBuilder::GenerateStarVertices(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points
    ) {
        std::vector<Point> vertices;
        vertices.reserve(static_cast<size_t>(points) * 2);

        float angleStep = PI / static_cast<float>(points);
        for (int i = 0; i < points * 2; ++i) {
            float r = (i % 2 == 0) ? outerRadius : innerRadius;
            float angle = i * angleStep;
            vertices.push_back({
                center.x + r * sinf(angle),
                center.y - r * cosf(angle)
                });
        }

        return vertices;
    }

    // center waveform vertically so oscillation is symmetric
    std::vector<Point> GeometryBuilder::GenerateWaveformPoints(
        const SpectrumData& spectrum,
        const Rect& bounds
    ) {
        std::vector<Point> points;
        if (spectrum.empty() || spectrum.size() < 2) {
            return points;
        }

        points.reserve(spectrum.size());
        float midline = bounds.y + bounds.height * 0.5f;
        float amplitude = bounds.height * 0.5f;
        float stepX = bounds.width / static_cast<float>(spectrum.size() - 1);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            points.push_back({
                bounds.x + i * stepX,
                midline - spectrum[i] * amplitude
                });
        }

        return points;
    }

} // namespace Spectrum