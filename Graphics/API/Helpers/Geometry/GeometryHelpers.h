#ifndef SPECTRUM_CPP_GEOMETRY_HELPERS_H
#define SPECTRUM_CPP_GEOMETRY_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Geometry Helpers - Common geometric operations
//
// This header provides utilities for working with basic geometric
// primitives (Point, Rect) commonly used in graphics and UI code.
//
// Key features:
// - Point operations (distance, lerp, arithmetic, angles)
// - Rect operations (center, containment, intersection)
// - Circle/ellipse operations (positions on circle, containment)
// - Hit-testing utilities (point in rect, point in circle)
// - Viewport utilities (center calculation, bounds creation)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Types.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace Spectrum::Helpers::Geometry {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Import Math utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    using Math::Lerp;
    using Math::Clamp;
    using Math::Saturate;
    using Math::Normalize;
    using Math::Map;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Point Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Calculates Euclidean distance between two points
    [[nodiscard]] inline float Distance(const Point& a, const Point& b) noexcept
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    /// Calculates squared distance (faster, for comparisons)
    [[nodiscard]] inline float DistanceSquared(const Point& a, const Point& b) noexcept
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        return dx * dx + dy * dy;
    }

    /// Point subtraction
    [[nodiscard]] inline Point Subtract(const Point& a, const Point& b) noexcept
    {
        return { a.x - b.x, a.y - b.y };
    }

    /// Point addition
    [[nodiscard]] inline Point Add(const Point& a, const Point& b) noexcept
    {
        return { a.x + b.x, a.y + b.y };
    }

    /// Point scalar multiplication
    [[nodiscard]] inline Point Multiply(const Point& p, float scalar) noexcept
    {
        return { p.x * scalar, p.y * scalar };
    }

    /// Point scalar division
    [[nodiscard]] inline Point Divide(const Point& p, float scalar) noexcept
    {
        if (std::abs(scalar) < 1e-6f) return { 0.0f, 0.0f };
        return { p.x / scalar, p.y / scalar };
    }

    /// Dot product of two vectors (points)
    [[nodiscard]] inline float Dot(const Point& a, const Point& b) noexcept
    {
        return a.x * b.x + a.y * b.y;
    }

    /// Vector length (magnitude)
    [[nodiscard]] inline float Length(const Point& p) noexcept
    {
        return std::sqrt(p.x * p.x + p.y * p.y);
    }

    /// Vector squared length (faster)
    [[nodiscard]] inline float LengthSquared(const Point& p) noexcept
    {
        return p.x * p.x + p.y * p.y;
    }

    /// Normalized vector (unit vector)
    [[nodiscard]] inline Point Normalize(const Point& p) noexcept
    {
        const float len = Length(p);
        if (len < 1e-6f) return { 0.0f, 0.0f };
        return { p.x / len, p.y / len };
    }

    /// Negates vector
    [[nodiscard]] inline Point Negate(const Point& p) noexcept
    {
        return { -p.x, -p.y };
    }

    /// Perpendicular vector (rotated 90 degrees counterclockwise)
    [[nodiscard]] inline Point Perpendicular(const Point& p) noexcept
    {
        return { -p.y, p.x };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Angle and Direction Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Calculates angle from point to another point (in radians)
    [[nodiscard]] inline float AngleBetween(const Point& from, const Point& to) noexcept
    {
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        return std::atan2(dy, dx);
    }

    /// Creates unit direction vector from angle (in radians)
    [[nodiscard]] inline Point DirectionFromAngle(float angleRadians) noexcept
    {
        return {
            std::cos(angleRadians),
            std::sin(angleRadians)
        };
    }

    /// Rotates point around origin by angle (in radians)
    [[nodiscard]] inline Point RotatePoint(const Point& p, float angleRadians) noexcept
    {
        const float cosA = std::cos(angleRadians);
        const float sinA = std::sin(angleRadians);
        return {
            p.x * cosA - p.y * sinA,
            p.x * sinA + p.y * cosA
        };
    }

    /// Rotates point around pivot by angle (in radians)
    [[nodiscard]] inline Point RotatePointAround(
        const Point& p,
        const Point& pivot,
        float angleRadians
    ) noexcept
    {
        const Point translated = Subtract(p, pivot);
        const Point rotated = RotatePoint(translated, angleRadians);
        return Add(rotated, pivot);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Circle Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Calculates point on circle at given angle (0 = right, increases counterclockwise)
    [[nodiscard]] inline Point PointOnCircle(
        const Point& center,
        float radius,
        float angleRadians
    ) noexcept
    {
        return {
            center.x + std::cos(angleRadians) * radius,
            center.y + std::sin(angleRadians) * radius
        };
    }

    /// Calculates point on ellipse at given angle
    [[nodiscard]] inline Point PointOnEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        float angleRadians
    ) noexcept
    {
        return {
            center.x + std::cos(angleRadians) * radiusX,
            center.y + std::sin(angleRadians) * radiusY
        };
    }

    /// Generates evenly distributed points on circle
    [[nodiscard]] inline std::vector<Point> GenerateCirclePoints(
        const Point& center,
        float radius,
        int pointCount
    )
    {
        std::vector<Point> points;
        points.reserve(pointCount);

        const float angleStep = TWO_PI / pointCount;

        for (int i = 0; i < pointCount; ++i) {
            const float angle = i * angleStep;
            points.push_back(PointOnCircle(center, radius, angle));
        }

        return points;
    }

    /// Calculates closest point on circle to given point
    [[nodiscard]] inline Point ClosestPointOnCircle(
        const Point& center,
        float radius,
        const Point& point
    ) noexcept
    {
        const Point direction = Subtract(point, center);
        const Point normalized = Normalize(direction);
        return Add(center, Multiply(normalized, radius));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rectangle Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Gets center point of rectangle
    [[nodiscard]] inline Point GetCenter(const Rect& rect) noexcept
    {
        return {
            rect.x + rect.width * 0.5f,
            rect.y + rect.height * 0.5f
        };
    }

    /// Gets right edge of rectangle
    [[nodiscard]] inline float GetRight(const Rect& rect) noexcept
    {
        return rect.x + rect.width;
    }

    /// Gets bottom edge of rectangle
    [[nodiscard]] inline float GetBottom(const Rect& rect) noexcept
    {
        return rect.y + rect.height;
    }

    /// Gets top-left corner
    [[nodiscard]] inline Point GetTopLeft(const Rect& rect) noexcept
    {
        return { rect.x, rect.y };
    }

    /// Gets top-right corner
    [[nodiscard]] inline Point GetTopRight(const Rect& rect) noexcept
    {
        return { GetRight(rect), rect.y };
    }

    /// Gets bottom-left corner
    [[nodiscard]] inline Point GetBottomLeft(const Rect& rect) noexcept
    {
        return { rect.x, GetBottom(rect) };
    }

    /// Gets bottom-right corner
    [[nodiscard]] inline Point GetBottomRight(const Rect& rect) noexcept
    {
        return { GetRight(rect), GetBottom(rect) };
    }

    /// Creates a rectangle centered at given point
    [[nodiscard]] inline Rect CreateCentered(
        const Point& center,
        float width,
        float height
    ) noexcept
    {
        return {
            center.x - width * 0.5f,
            center.y - height * 0.5f,
            width,
            height
        };
    }

    /// Creates a square centered at given point
    [[nodiscard]] inline Rect CreateCenteredSquare(
        const Point& center,
        float size
    ) noexcept
    {
        return CreateCentered(center, size, size);
    }

    /// Creates rectangle from two points (top-left and bottom-right)
    [[nodiscard]] inline Rect CreateFromPoints(
        const Point& topLeft,
        const Point& bottomRight
    ) noexcept
    {
        return {
            topLeft.x,
            topLeft.y,
            bottomRight.x - topLeft.x,
            bottomRight.y - topLeft.y
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Viewport Utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Gets center point of viewport
    [[nodiscard]] inline Point GetViewportCenter(int width, int height) noexcept
    {
        return {
            static_cast<float>(width) * 0.5f,
            static_cast<float>(height) * 0.5f
        };
    }

    /// Gets center point of viewport (float version)
    [[nodiscard]] inline Point GetViewportCenter(float width, float height) noexcept
    {
        return { width * 0.5f, height * 0.5f };
    }

    /// Creates viewport bounds rectangle
    [[nodiscard]] inline Rect CreateViewportBounds(int width, int height) noexcept
    {
        return {
            0.0f,
            0.0f,
            static_cast<float>(width),
            static_cast<float>(height)
        };
    }

    /// Creates viewport bounds rectangle (float version)
    [[nodiscard]] inline Rect CreateViewportBounds(float width, float height) noexcept
    {
        return { 0.0f, 0.0f, width, height };
    }

    /// Calculates maximum radius that fits in viewport
    [[nodiscard]] inline float GetMaxRadiusInViewport(int width, int height) noexcept
    {
        return std::min(width, height) * 0.5f;
    }

    /// Calculates maximum radius that fits in viewport (float version)
    [[nodiscard]] inline float GetMaxRadiusInViewport(float width, float height) noexcept
    {
        return std::min(width, height) * 0.5f;
    }

    /// Calculates maximum radius with padding
    [[nodiscard]] inline float GetMaxRadiusWithPadding(
        int width,
        int height,
        float padding
    ) noexcept
    {
        return std::min(width, height) * 0.5f - padding;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Containment Tests
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Checks if point is inside rectangle
    [[nodiscard]] inline bool Contains(const Rect& rect, const Point& point) noexcept
    {
        return point.x >= rect.x &&
            point.x <= GetRight(rect) &&
            point.y >= rect.y &&
            point.y <= GetBottom(rect);
    }

    /// Checks if point is inside rectangle (with padding)
    [[nodiscard]] inline bool ContainsWithPadding(
        const Rect& rect,
        const Point& point,
        float padding
    ) noexcept
    {
        return point.x >= (rect.x - padding) &&
            point.x <= (GetRight(rect) + padding) &&
            point.y >= (rect.y - padding) &&
            point.y <= (GetBottom(rect) + padding);
    }

    /// Checks if point is inside circle
    [[nodiscard]] inline bool ContainsCircle(
        const Point& center,
        float radius,
        const Point& point
    ) noexcept
    {
        return DistanceSquared(center, point) <= (radius * radius);
    }

    /// Checks if point is inside ellipse
    [[nodiscard]] inline bool ContainsEllipse(
        const Point& center,
        float radiusX,
        float radiusY,
        const Point& point
    ) noexcept
    {
        const float dx = point.x - center.x;
        const float dy = point.y - center.y;
        return (dx * dx) / (radiusX * radiusX) + (dy * dy) / (radiusY * radiusY) <= 1.0f;
    }

    /// Checks if rectangles intersect
    [[nodiscard]] inline bool Intersects(const Rect& a, const Rect& b) noexcept
    {
        return !(GetRight(a) < b.x ||
            a.x > GetRight(b) ||
            GetBottom(a) < b.y ||
            a.y > GetBottom(b));
    }

    /// Checks if rectangle fully contains another rectangle
    [[nodiscard]] inline bool ContainsRect(const Rect& outer, const Rect& inner) noexcept
    {
        return inner.x >= outer.x &&
            GetRight(inner) <= GetRight(outer) &&
            inner.y >= outer.y &&
            GetBottom(inner) <= GetBottom(outer);
    }

    /// Checks if point is within bounds
    [[nodiscard]] inline bool IsInBounds(
        const Point& point,
        float minX,
        float maxX,
        float minY,
        float maxY
    ) noexcept
    {
        return point.x >= minX && point.x <= maxX &&
            point.y >= minY && point.y <= maxY;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transformations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Offsets rectangle by delta
    [[nodiscard]] inline Rect Offset(const Rect& rect, float dx, float dy) noexcept
    {
        return { rect.x + dx, rect.y + dy, rect.width, rect.height };
    }

    /// Offsets rectangle by point delta
    [[nodiscard]] inline Rect OffsetBy(const Rect& rect, const Point& delta) noexcept
    {
        return Offset(rect, delta.x, delta.y);
    }

    /// Inflates rectangle by amount (expands from center)
    [[nodiscard]] inline Rect Inflate(const Rect& rect, float amount) noexcept
    {
        return {
            rect.x - amount,
            rect.y - amount,
            rect.width + amount * 2.0f,
            rect.height + amount * 2.0f
        };
    }

    /// Inflates rectangle by different amounts for X and Y
    [[nodiscard]] inline Rect InflateXY(
        const Rect& rect,
        float amountX,
        float amountY
    ) noexcept
    {
        return {
            rect.x - amountX,
            rect.y - amountY,
            rect.width + amountX * 2.0f,
            rect.height + amountY * 2.0f
        };
    }

    /// Deflates rectangle by amount (shrinks from center)
    [[nodiscard]] inline Rect Deflate(const Rect& rect, float amount) noexcept
    {
        return Inflate(rect, -amount);
    }

    /// Scales rectangle from center
    [[nodiscard]] inline Rect Scale(const Rect& rect, float scale) noexcept
    {
        const Point center = GetCenter(rect);
        const float newWidth = rect.width * scale;
        const float newHeight = rect.height * scale;
        return CreateCentered(center, newWidth, newHeight);
    }

    /// Scales rectangle with different factors for width/height
    [[nodiscard]] inline Rect ScaleXY(
        const Rect& rect,
        float scaleX,
        float scaleY
    ) noexcept
    {
        const Point center = GetCenter(rect);
        const float newWidth = rect.width * scaleX;
        const float newHeight = rect.height * scaleY;
        return CreateCentered(center, newWidth, newHeight);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Clamping & Constraints
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Clamps point to stay within rectangle bounds
    [[nodiscard]] inline Point ClampToRect(const Point& point, const Rect& bounds) noexcept
    {
        return {
            Clamp(point.x, bounds.x, GetRight(bounds)),
            Clamp(point.y, bounds.y, GetBottom(bounds))
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Checks if rect has valid dimensions
    [[nodiscard]] inline bool IsValid(const Rect& rect) noexcept
    {
        return rect.width > 0.0f && rect.height > 0.0f;
    }

    /// Checks if rect has zero or negative area
    [[nodiscard]] inline bool IsEmpty(const Rect& rect) noexcept
    {
        return rect.width <= 0.0f || rect.height <= 0.0f;
    }

    /// Gets area of rectangle
    [[nodiscard]] inline float Area(const Rect& rect) noexcept
    {
        return rect.width * rect.height;
    }

    /// Gets perimeter of rectangle
    [[nodiscard]] inline float Perimeter(const Rect& rect) noexcept
    {
        return 2.0f * (rect.width + rect.height);
    }

    /// Gets aspect ratio (width / height)
    [[nodiscard]] inline float AspectRatio(const Rect& rect) noexcept
    {
        return rect.height > 0.0f ? rect.width / rect.height : 0.0f;
    }

    /// Computes union of two rectangles (smallest rect containing both)
    [[nodiscard]] inline Rect Union(const Rect& a, const Rect& b) noexcept
    {
        const float left = std::min(a.x, b.x);
        const float top = std::min(a.y, b.y);
        const float right = std::max(GetRight(a), GetRight(b));
        const float bottom = std::max(GetBottom(a), GetBottom(b));

        return {
            left,
            top,
            right - left,
            bottom - top
        };
    }

    /// Computes intersection of two rectangles
    [[nodiscard]] inline Rect Intersection(const Rect& a, const Rect& b) noexcept
    {
        const float left = std::max(a.x, b.x);
        const float top = std::max(a.y, b.y);
        const float right = std::min(GetRight(a), GetRight(b));
        const float bottom = std::min(GetBottom(a), GetBottom(b));

        if (left >= right || top >= bottom) {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        return {
            left,
            top,
            right - left,
            bottom - top
        };
    }

    /// Calculates closest point on rectangle to given point
    [[nodiscard]] inline Point ClosestPointOnRect(
        const Rect& rect,
        const Point& point
    ) noexcept
    {
        return {
            Clamp(point.x, rect.x, GetRight(rect)),
            Clamp(point.y, rect.y, GetBottom(rect))
        };
    }

    /// Calculates distance from point to rectangle edge (0 if inside)
    [[nodiscard]] inline float DistanceToRect(
        const Point& point,
        const Rect& rect
    ) noexcept
    {
        if (Contains(rect, point)) {
            return 0.0f;
        }

        const Point closest = ClosestPointOnRect(rect, point);
        return Distance(point, closest);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Batch Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Calculates bounding rectangle for a set of points
    [[nodiscard]] inline Rect BoundingRect(const std::vector<Point>& points)
    {
        if (points.empty()) {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        float minX = points[0].x;
        float maxX = points[0].x;
        float minY = points[0].y;
        float maxY = points[0].y;

        for (const auto& p : points) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }

        return {
            minX,
            minY,
            maxX - minX,
            maxY - minY
        };
    }

    /// Transforms all points by offset
    [[nodiscard]] inline std::vector<Point> OffsetPoints(
        const std::vector<Point>& points,
        const Point& offset
    )
    {
        std::vector<Point> result;
        result.reserve(points.size());

        for (const auto& p : points) {
            result.push_back(Add(p, offset));
        }

        return result;
    }

    /// Scales all points from origin
    [[nodiscard]] inline std::vector<Point> ScalePoints(
        const std::vector<Point>& points,
        float scale
    )
    {
        std::vector<Point> result;
        result.reserve(points.size());

        for (const auto& p : points) {
            result.push_back(Multiply(p, scale));
        }

        return result;
    }

} // namespace Spectrum::Helpers::Geometry

#endif // SPECTRUM_CPP_GEOMETRY_HELPERS_H