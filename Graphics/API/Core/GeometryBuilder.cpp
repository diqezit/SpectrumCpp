// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the GeometryBuilder class for Direct2D geometry creation.
//
// Implementation details:
// - Uses Direct2D factory to build complex path geometries
// - Provides optimized vertex generation for common shapes
// - Handles Direct2D arc rendering quirks (large arc flag)
// - Uses D2DHelpers for conversion, sanitization, and geometry helpers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Core/GeometryBuilder.h"
#include "Graphics/API/D2DHelpers.h"
#include <cmath>

namespace Spectrum {

    using namespace D2DHelpers;
    using namespace Helpers::TypeConversion;
    using namespace Helpers::Math;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;
    using namespace Helpers::HResult;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GeometryBuilder::GeometryBuilder(ID2D1Factory* factory)
        : m_factory(factory)
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Path Geometry Creation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreatePathFromPoints(
        const std::vector<Point>& points,
        bool closed,
        bool filled
    ) const
    {
        if (!PointArray(points, 2) || !m_factory) return nullptr;

        wrl::ComPtr<ID2D1PathGeometry> geometry;
        HRESULT hr = m_factory->CreatePathGeometry(geometry.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1Factory::CreatePathGeometry", geometry)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geometry->Open(sink.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1PathGeometry::Open", sink)) {
            return nullptr;
        }

        Geometry::BeginFigure(sink.Get(), points[0], filled);

        for (size_t i = 1; i < points.size(); ++i) {
            Geometry::AddLine(sink.Get(), points[i]);
        }

        Geometry::EndFigure(sink.Get(), closed);

        sink->Close();
        return geometry;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle
    ) const
    {
        if (!m_factory) return nullptr;
        if (!PositiveRadius(radius)) return nullptr;
        if (!NonZeroAngle(sweepAngle)) return nullptr;

        wrl::ComPtr<ID2D1PathGeometry> geometry;
        HRESULT hr = m_factory->CreatePathGeometry(geometry.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1Factory::CreatePathGeometry", geometry)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geometry->Open(sink.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1PathGeometry::Open", sink)) {
            return nullptr;
        }

        const float startRad = DegreesToRadians(startAngle);
        const float endRad = DegreesToRadians(startAngle + sweepAngle);

        const Point startPoint = {
            center.x + radius * std::cos(startRad),
            center.y + radius * std::sin(startRad)
        };

        Geometry::BeginFigure(sink.Get(), startPoint, false);

        D2D1_ARC_SEGMENT arc = {};
        arc.size = ToD2DSize(radius, radius);
        arc.rotationAngle = 0.0f;
        arc.sweepDirection = (sweepAngle > 0.0f)
            ? D2D1_SWEEP_DIRECTION_CLOCKWISE
            : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

        arc.arcSize = (std::abs(sweepAngle) >= 180.0f)
            ? D2D1_ARC_SIZE_LARGE
            : D2D1_ARC_SIZE_SMALL;

        arc.point = {
            center.x + radius * std::cos(endRad),
            center.y + radius * std::sin(endRad)
        };

        sink->AddArc(&arc);
        Geometry::EndFigure(sink.Get(), false);
        sink->Close();

        return geometry;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation
    ) const
    {
        if (!m_factory) return nullptr;
        if (!PositiveRadius(radius)) return nullptr;

        const int sanitizedSides = PolygonSides(sides);
        const auto vertices = GenerateRegularPolygonVertices(center, radius, sanitizedSides, rotation);

        wrl::ComPtr<ID2D1PathGeometry> geometry;
        HRESULT hr = m_factory->CreatePathGeometry(geometry.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1Factory::CreatePathGeometry", geometry)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geometry->Open(sink.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1PathGeometry::Open", sink)) {
            return nullptr;
        }

        Geometry::BeginFigure(sink.Get(), vertices.front(), true);

        for (size_t i = 1; i < vertices.size(); ++i) {
            Geometry::AddLine(sink.Get(), vertices[i]);
        }

        Geometry::EndFigure(sink.Get(), true);
        sink->Close();

        return geometry;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateAngularSlice(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle
    ) const
    {
        if (!m_factory) return nullptr;
        if (!PositiveRadius(radius)) return nullptr;

        wrl::ComPtr<ID2D1PathGeometry> geometry;
        HRESULT hr = m_factory->CreatePathGeometry(geometry.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1Factory::CreatePathGeometry", geometry)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geometry->Open(sink.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1PathGeometry::Open", sink)) {
            return nullptr;
        }

        Geometry::BeginFigure(sink.Get(), center, true);

        const Point startPoint = {
            center.x + radius * std::cos(DegreesToRadians(startAngle)),
            center.y + radius * std::sin(DegreesToRadians(startAngle))
        };
        Geometry::AddLine(sink.Get(), startPoint);

        const Point endPoint = {
            center.x + radius * std::cos(DegreesToRadians(endAngle)),
            center.y + radius * std::sin(DegreesToRadians(endAngle))
        };
        Geometry::AddLine(sink.Get(), endPoint);

        Geometry::EndFigure(sink.Get(), true);
        sink->Close();

        return geometry;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Vertex Generation (Static Methods)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    std::vector<Point> GeometryBuilder::GenerateCirclePoints(
        const Point& center,
        float radius,
        int segments
    )
    {
        const int sanitizedSegments = CircleSegments(segments);

        std::vector<Point> points;
        points.reserve(static_cast<size_t>(sanitizedSegments) + 1);

        const float angleStep = kTwoPi / static_cast<float>(sanitizedSegments);

        for (int i = 0; i <= sanitizedSegments; ++i) {
            const float angle = i * angleStep;
            points.push_back({
                center.x + radius * std::cos(angle),
                center.y + radius * std::sin(angle)
                });
        }

        return points;
    }

    std::vector<Point> GeometryBuilder::GenerateRegularPolygonVertices(
        const Point& center,
        float radius,
        int sides,
        float rotation
    )
    {
        const int sanitizedSides = PolygonSides(sides);
        std::vector<Point> vertices;
        vertices.reserve(sanitizedSides);

        const float angleStep = kTwoPi / static_cast<float>(sanitizedSides);
        const float rotationRad = DegreesToRadians(rotation);

        for (int i = 0; i < sanitizedSides; ++i) {
            const float angle = i * angleStep + rotationRad;
            vertices.push_back({
                center.x + radius * std::cos(angle),
                center.y + radius * std::sin(angle)
                });
        }
        return vertices;
    }

    std::vector<Point> GeometryBuilder::GenerateStarVertices(
        const Point& center,
        float outerRadius,
        float innerRadius,
        int points
    )
    {
        const int sanitizedPoints = StarPoints(points);

        std::vector<Point> vertices;
        vertices.reserve(static_cast<size_t>(sanitizedPoints) * 2);

        const float angleStep = kPi / static_cast<float>(sanitizedPoints);

        for (int i = 0; i < sanitizedPoints * 2; ++i) {
            const float radius = (i % 2 == 0) ? outerRadius : innerRadius;
            const float angle = i * angleStep;

            vertices.push_back({
                center.x + radius * std::sin(angle),
                center.y - radius * std::cos(angle)
                });
        }

        return vertices;
    }

    std::vector<Point> GeometryBuilder::GenerateWaveformPoints(
        const SpectrumData& spectrum,
        const Rect& bounds
    )
    {
        if (spectrum.empty() || spectrum.size() < 2) return {};

        std::vector<Point> points;
        points.reserve(spectrum.size());

        const float midline = bounds.y + bounds.height * 0.5f;
        const float amplitude = bounds.height * 0.5f;
        const float stepX = bounds.width / static_cast<float>(spectrum.size() - 1);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float sanitizedValue = NormalizedFloat(spectrum[i]);
            points.push_back({
                bounds.x + i * stepX,
                midline - sanitizedValue * amplitude
                });
        }

        return points;
    }

} // namespace Spectrum