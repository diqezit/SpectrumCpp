// GradientRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the GradientRenderer class. This file contains logic for
// drawing shapes with linear, radial, and simulated angular gradients.
//
// Implementation details:
// - Gradient keys generated via optimized ostringstream for cache lookup
// - Angular gradients simulated via multiple geometry segments
// - All gradient stop collections validated before use
// - Uses D2DHelpers for sanitization, validation, and type conversion
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GradientRenderer.h"
#include "D2DHelpers.h"
#include "ColorUtils.h"
#include <sstream>
#include <iomanip>

namespace Spectrum {

    using namespace D2DHelpers;

    namespace {

        [[nodiscard]] std::string GenerateGradientKey(
            std::string_view prefix,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        )
        {
            std::ostringstream oss;
            oss << prefix << "_";
            oss << std::fixed << std::setprecision(2);

            for (const auto& stop : stops) {
                oss << stop.position << "_"
                    << stop.color.r << "_"
                    << stop.color.g << "_"
                    << stop.color.b << "_";
            }

            return oss.str();
        }

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GradientRenderer::GradientRenderer(
        ID2D1RenderTarget* renderTarget,
        ID2D1SolidColorBrush* solidBrush,
        ResourceCache* cache,
        GeometryBuilder* geometryBuilder
    )
        : m_renderTarget(renderTarget)
        , m_solidBrush(solidBrush)
        , m_cache(cache)
        , m_geometryBuilder(geometryBuilder)
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradient Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GradientRenderer::DrawGradientRectangle(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool horizontal
    ) const
    {
        if (!m_renderTarget || !m_cache) return;
        if (!Validate::GradientStops(stops)) return;

        const Point start = { rect.x, rect.y };
        const Point end = horizontal
            ? Point{ rect.GetRight(), rect.y }
        : Point{ rect.x, rect.GetBottom() };

        const std::string key = GenerateGradientKey("gradient_rect", stops);
        auto* brush = m_cache->GetLinearGradient(key, start, end, stops);

        if (!brush) return;

        const D2D1_RECT_F d2dRect = ToD2DRect(rect);
        m_renderTarget->FillRectangle(&d2dRect, brush);
    }

    void GradientRenderer::DrawRadialGradient(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) const
    {
        if (!m_renderTarget || !m_cache) return;
        if (!Validate::GradientStops(stops)) return;
        if (!Validate::PositiveRadius(radius)) return;

        const std::string key = GenerateGradientKey("radial_gradient", stops);
        auto* brush = m_cache->GetRadialGradient(key, center, radius, stops);

        if (!brush) return;

        const D2D1_ELLIPSE ellipse = {
            ToD2DPoint(center),
            radius,
            radius
        };

        m_renderTarget->FillEllipse(&ellipse, brush);
    }

    void GradientRenderer::DrawGradientCircle(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool filled
    ) const
    {
        if (!Validate::GradientStops(stops)) return;

        if (filled) {
            DrawRadialGradient(center, radius, stops);
            return;
        }

        if (!Validate::RenderTargetAndBrush(m_renderTarget, m_solidBrush)) return;

        const auto& backColor = stops.back().color;
        const Color color = { backColor.r, backColor.g, backColor.b, backColor.a };

        constexpr float strokeWidth = 2.0f;
        const float adjustedRadius = Sanitize::PositiveFloat(radius - strokeWidth * 0.5f, 0.0f);

        if (!Validate::PositiveRadius(adjustedRadius)) return;

        m_solidBrush->SetColor(ToD2DColor(color));

        const D2D1_ELLIPSE ellipse = {
            ToD2DPoint(center),
            adjustedRadius,
            adjustedRadius
        };

        m_renderTarget->DrawEllipse(&ellipse, m_solidBrush, strokeWidth);
    }

    void GradientRenderer::DrawGradientPath(
        const std::vector<Point>& points,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float strokeWidth
    ) const
    {
        if (!m_renderTarget || !m_cache || !m_geometryBuilder) return;
        if (!Validate::PointArray(points, 2)) return;
        if (!Validate::GradientStops(stops)) return;

        const std::string key = GenerateGradientKey("path_gradient", stops);
        auto* brush = m_cache->GetLinearGradient(
            key,
            points.front(),
            points.back(),
            stops
        );

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);

        if (!brush || !geo) return;

        m_renderTarget->DrawGeometry(geo.Get(), brush, strokeWidth);
    }

    void GradientRenderer::DrawAngularGradient(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle,
        const Color& startColor,
        const Color& endColor
    ) const
    {
        if (!Validate::RenderTargetAndBrush(m_renderTarget, m_solidBrush)) return;
        if (!m_geometryBuilder) return;
        if (!Validate::PositiveRadius(radius)) return;

        const float sweep = endAngle - startAngle;
        if (!Validate::NonZeroAngle(sweep)) return;

        constexpr int kDefaultSegments = 180;
        const int segments = Sanitize::CircleSegments(kDefaultSegments);

        const float angleStep = sweep / static_cast<float>(segments);

        for (int i = 0; i < segments; ++i) {
            const float a0 = startAngle + i * angleStep;
            const float a1 = startAngle + (i + 1) * angleStep;

            auto geo = m_geometryBuilder->CreateAngularSlice(center, radius, a0, a1);
            if (!geo) continue;

            const float t = (static_cast<float>(i) + 0.5f) / segments;
            const Color midColor = Utils::InterpolateColor(startColor, endColor, t);

            m_solidBrush->SetColor(ToD2DColor(midColor));
            m_renderTarget->FillGeometry(geo.Get(), m_solidBrush);
        }
    }

    void GradientRenderer::DrawVerticalGradientBar(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float cornerRadius
    ) const
    {
        if (!m_renderTarget || !m_cache) return;
        if (!Validate::GradientStops(stops)) return;

        const std::string key = GenerateGradientKey("vbar_gradient", stops);
        auto* brush = m_cache->GetLinearGradient(
            key,
            { rect.x, rect.y },
            { rect.x, rect.GetBottom() },
            stops
        );

        if (!brush) return;

        const float sanitizedRadius = Sanitize::NonNegativeFloat(cornerRadius);

        if (sanitizedRadius > 0.0f) {
            const D2D1_ROUNDED_RECT rr = {
                ToD2DRect(rect),
                sanitizedRadius,
                sanitizedRadius
            };
            m_renderTarget->FillRoundedRectangle(&rr, brush);
        }
        else {
            const D2D1_RECT_F d2dRect = ToD2DRect(rect);
            m_renderTarget->FillRectangle(&d2dRect, brush);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GradientRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget)
    {
        m_renderTarget = renderTarget;
    }

} // namespace Spectrum