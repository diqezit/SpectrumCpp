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
    using namespace Helpers::HResult;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr int INCREMENTAL_THRESHOLD = 8;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GeometryBuilder::GeometryBuilder(wrl::ComPtr<ID2D1Factory> factory)
        : m_factory(std::move(factory))
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
        if (!ValidatePointArray(points, 2)) return nullptr;

        auto geometry = CreateEmptyGeometry();
        if (!geometry) return nullptr;

        auto sink = OpenGeometrySink(geometry.Get());
        if (!sink) return nullptr;

        BuildPathFromPoints(sink.Get(), points, closed, filled);

        if (!CloseGeometrySink(sink.Get())) return nullptr;

        return geometry;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateArc(
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle
    ) const
    {
        if (!ValidateArcParameters(radius, sweepAngle)) return nullptr;

        auto geometry = CreateEmptyGeometry();
        if (!geometry) return nullptr;

        auto sink = OpenGeometrySink(geometry.Get());
        if (!sink) return nullptr;

        BuildArcPath(sink.Get(), center, radius, startAngle, sweepAngle);

        if (!CloseGeometrySink(sink.Get())) return nullptr;

        return geometry;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateRegularPolygon(
        const Point& center,
        float radius,
        int sides,
        float rotation
    ) const
    {
        if (!CanCreateGeometry()) return nullptr;
        if (!Helpers::Validate::PositiveRadius(radius)) return nullptr;

        const int sanitizedSides = Helpers::Sanitize::PolygonSides(sides);
        const auto vertices = GenerateRegularPolygonVertices(center, radius, sanitizedSides, rotation);

        auto geometry = CreateEmptyGeometry();
        if (!geometry) return nullptr;

        auto sink = OpenGeometrySink(geometry.Get());
        if (!sink) return nullptr;

        BuildPolygonPath(sink.Get(), vertices);

        if (!CloseGeometrySink(sink.Get())) return nullptr;

        return geometry;
    }

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateAngularSlice(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle
    ) const
    {
        if (!ValidateSliceParameters(radius)) return nullptr;

        auto geometry = CreateEmptyGeometry();
        if (!geometry) return nullptr;

        auto sink = OpenGeometrySink(geometry.Get());
        if (!sink) return nullptr;

        BuildSlicePath(sink.Get(), center, radius, startAngle, endAngle);

        if (!CloseGeometrySink(sink.Get())) return nullptr;

        return geometry;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Geometry Creation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1PathGeometry> GeometryBuilder::CreateEmptyGeometry() const
    {
        if (!CanCreateGeometry()) return nullptr;

        wrl::ComPtr<ID2D1PathGeometry> geometry;
        HRESULT hr = m_factory->CreatePathGeometry(geometry.GetAddressOf());

        if (!CheckComCreation(hr, "ID2D1Factory::CreatePathGeometry", geometry)) {
            return nullptr;
        }

        return geometry;
    }

    wrl::ComPtr<ID2D1GeometrySink> GeometryBuilder::OpenGeometrySink(
        ID2D1PathGeometry* geometry
    ) const
    {
        if (!geometry) return nullptr;

        wrl::ComPtr<ID2D1GeometrySink> sink;
        HRESULT hr = geometry->Open(sink.GetAddressOf());

        if (!CheckComCreation(hr, "ID2D1PathGeometry::Open", sink)) {
            return nullptr;
        }

        return sink;
    }

    bool GeometryBuilder::CloseGeometrySink(ID2D1GeometrySink* sink) const
    {
        if (!sink) return false;

        HRESULT hr = sink->Close();
        return SUCCEEDED(hr);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Path Building
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GeometryBuilder::BuildPathFromPoints(
        ID2D1GeometrySink* sink,
        const std::vector<Point>& points,
        bool closed,
        bool filled
    ) const
    {
        Geometry::BeginFigure(sink, points[0], filled);

        for (size_t i = 1; i < points.size(); ++i) {
            Geometry::AddLine(sink, points[i]);
        }

        Geometry::EndFigure(sink, closed);
    }

    void GeometryBuilder::BuildArcPath(
        ID2D1GeometrySink* sink,
        const Point& center,
        float radius,
        float startAngle,
        float sweepAngle
    ) const
    {
        const Point startPoint = CalculatePointOnCircle(center, radius, startAngle);

        Geometry::BeginFigure(sink, startPoint, false);

        const Point endPoint = CalculatePointOnCircle(center, radius, startAngle + sweepAngle);
        const D2D1_ARC_SEGMENT arc = CreateArcSegment(endPoint, radius, sweepAngle);

        sink->AddArc(&arc);
        Geometry::EndFigure(sink, false);
    }

    void GeometryBuilder::BuildPolygonPath(
        ID2D1GeometrySink* sink,
        const std::vector<Point>& vertices
    ) const
    {
        if (vertices.empty()) return;

        Geometry::BeginFigure(sink, vertices.front(), true);

        for (size_t i = 1; i < vertices.size(); ++i) {
            Geometry::AddLine(sink, vertices[i]);
        }

        Geometry::EndFigure(sink, true);
    }

    void GeometryBuilder::BuildSlicePath(
        ID2D1GeometrySink* sink,
        const Point& center,
        float radius,
        float startAngle,
        float endAngle
    ) const
    {
        Geometry::BeginFigure(sink, center, true);

        const Point startPoint = CalculatePointOnCircle(center, radius, startAngle);
        Geometry::AddLine(sink, startPoint);

        const Point endPoint = CalculatePointOnCircle(center, radius, endAngle);
        const float sweepAngle = endAngle - startAngle;
        const D2D1_ARC_SEGMENT arc = CreateArcSegment(endPoint, radius, sweepAngle);

        sink->AddArc(&arc);
        Geometry::EndFigure(sink, true);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Arc Segment Creation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    D2D1_ARC_SEGMENT GeometryBuilder::CreateArcSegment(
        const Point& endPoint,
        float radius,
        float sweepAngle
    ) const
    {
        D2D1_ARC_SEGMENT arc = {};
        arc.size = D2D1::SizeF(radius, radius);
        arc.rotationAngle = 0.0f;
        arc.sweepDirection = (sweepAngle > 0.0f)
            ? D2D1_SWEEP_DIRECTION_CLOCKWISE
            : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
        arc.arcSize = (std::abs(sweepAngle) >= 180.0f)
            ? D2D1_ARC_SIZE_LARGE
            : D2D1_ARC_SIZE_SMALL;
        arc.point = D2D1::Point2F(endPoint.x, endPoint.y);

        return arc;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Point Calculation
    // =-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Point GeometryBuilder::CalculatePointOnCircle(
        const Point& center,
        float radius,
        float angleDegrees
    ) const
    {
        const float angleRad = DegreesToRadians(angleDegrees);
        return CalculatePointOnCircleRad(center, radius, angleRad);
    }

    Point GeometryBuilder::CalculatePointOnCircleRad(
        const Point& center,
        float radius,
        float angleRadians
    ) const
    {
        return {
            center.x + radius * std::cos(angleRadians),
            center.y + radius * std::sin(angleRadians)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Vertex Generation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GeometryBuilder::RotationState GeometryBuilder::InitializeRotation(
        float startAngle,
        float angleStep
    )
    {
        return {
            std::cos(startAngle),
            std::sin(startAngle),
            std::cos(angleStep),
            std::sin(angleStep)
        };
    }

    void GeometryBuilder::ApplyRotation(RotationState& state)
    {
        const float nextCos = state.cosAngle * state.cosStep - state.sinAngle * state.sinStep;
        const float nextSin = state.sinAngle * state.cosStep + state.cosAngle * state.sinStep;

        state.cosAngle = nextCos;
        state.sinAngle = nextSin;
    }

    Point GeometryBuilder::CalculatePointFromRotation(
        const Point& center,
        float radius,
        const RotationState& state
    )
    {
        return {
            center.x + radius * state.cosAngle,
            center.y + radius * state.sinAngle
        };
    }

    void GeometryBuilder::GeneratePointsDirect(
        std::vector<Point>& points,
        const Point& center,
        float radius,
        int count,
        float startAngle,
        float angleStep
    )
    {
        for (int i = 0; i < count; ++i) {
            const float angle = startAngle + i * angleStep;
            points.emplace_back(
                center.x + radius * std::cos(angle),
                center.y + radius * std::sin(angle)
            );
        }
    }

    void GeometryBuilder::GeneratePointsIncremental(
        std::vector<Point>& points,
        const Point& center,
        float radius,
        int count,
        float startAngle,
        float angleStep
    )
    {
        RotationState state = InitializeRotation(startAngle, angleStep);

        for (int i = 0; i < count; ++i) {
            points.emplace_back(CalculatePointFromRotation(center, radius, state));
            ApplyRotation(state);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool GeometryBuilder::ValidatePointArray(
        const std::vector<Point>& points,
        size_t minPoints
    ) const noexcept
    {
        return CanCreateGeometry() && Helpers::Validate::PointArray(points, minPoints);
    }

    bool GeometryBuilder::ValidateArcParameters(
        float radius,
        float sweepAngle
    ) const noexcept
    {
        return CanCreateGeometry() &&
            Helpers::Validate::PositiveRadius(radius) &&
            Helpers::Validate::NonZeroAngle(sweepAngle);
    }

    bool GeometryBuilder::ValidateSliceParameters(
        float radius
    ) const noexcept
    {
        return CanCreateGeometry() && Helpers::Validate::PositiveRadius(radius);
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
        const int sanitizedSegments = Helpers::Sanitize::CircleSegments(segments);

        std::vector<Point> points;
        points.reserve(static_cast<size_t>(sanitizedSegments) + 1);

        const float angleStep = TWO_PI / static_cast<float>(sanitizedSegments);

        if (sanitizedSegments <= INCREMENTAL_THRESHOLD) {
            GeneratePointsDirect(points, center, radius, sanitizedSegments + 1, 0.0f, angleStep);
        }
        else {
            GeneratePointsIncremental(points, center, radius, sanitizedSegments + 1, 0.0f, angleStep);
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
        const int sanitizedSides = Helpers::Sanitize::PolygonSides(sides);

        std::vector<Point> vertices;
        vertices.reserve(sanitizedSides);

        const float angleStep = TWO_PI / static_cast<float>(sanitizedSides);
        const float rotationRad = DegreesToRadians(rotation);

        if (sanitizedSides <= INCREMENTAL_THRESHOLD) {
            GeneratePointsDirect(vertices, center, radius, sanitizedSides, rotationRad, angleStep);
        }
        else {
            GeneratePointsIncremental(vertices, center, radius, sanitizedSides, rotationRad, angleStep);
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
        const int sanitizedPoints = Helpers::Sanitize::StarPoints(points);
        const int totalVertices = sanitizedPoints * 2;

        std::vector<Point> vertices;
        vertices.reserve(static_cast<size_t>(totalVertices));

        const float angleStep = PI / static_cast<float>(sanitizedPoints);

        if (totalVertices <= INCREMENTAL_THRESHOLD * 2) {
            for (int i = 0; i < totalVertices; ++i) {
                const float radius = (i & 1) ? innerRadius : outerRadius;
                const float angle = i * angleStep;

                vertices.emplace_back(
                    center.x + radius * std::sin(angle),
                    center.y - radius * std::cos(angle)
                );
            }
        }
        else {
            RotationState state = InitializeRotation(-PI / 2.0f, angleStep);

            for (int i = 0; i < totalVertices; ++i) {
                const float radius = (i & 1) ? innerRadius : outerRadius;

                vertices.emplace_back(
                    center.x + radius * state.sinAngle,
                    center.y + radius * state.cosAngle
                );

                ApplyRotation(state);
            }
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
            const float sanitizedValue = Helpers::Sanitize::NormalizedFloat(spectrum[i]);
            points.emplace_back(
                bounds.x + i * stepX,
                midline - sanitizedValue * amplitude
            );
        }

        return points;
    }

} // namespace Spectrum