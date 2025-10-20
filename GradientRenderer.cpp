// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the GradientRenderer class
// It provides methods for drawing shapes with linear, radial, and
// simulated angular gradients to create rich visual styles
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GradientRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include <sstream>
#include <iomanip>

namespace Spectrum {

    namespace {
        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }

        inline D2D1_RECT_F ToD2DRect(const Rect& r) {
            return D2D1::RectF(r.x, r.y, r.GetRight(), r.GetBottom());
        }

        std::string GenerateGradientKey(
            const std::string& prefix,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) {
            std::ostringstream oss;
            oss << prefix << "_";

            for (const auto& stop : stops) {
                oss << std::fixed << std::setprecision(2)
                    << stop.position << "_"
                    << stop.color.r << "_"
                    << stop.color.g << "_"
                    << stop.color.b << "_";
            }

            return oss.str();
        }
    }

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

    void GradientRenderer::DrawGradientRectangle(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool horizontal
    ) {
        if (!m_renderTarget || !m_cache || stops.empty()) {
            return;
        }

        Point start = { rect.x, rect.y };
        Point end = horizontal
            ? Point{ rect.GetRight(), rect.y }
        : Point{ rect.x, rect.GetBottom() };

        std::string key = GenerateGradientKey("gradient_rect", stops);
        auto* brush = m_cache->GetLinearGradient(key, start, end, stops);
        if (brush) {
            D2D1_RECT_F d2dRect = ToD2DRect(rect);
            m_renderTarget->FillRectangle(&d2dRect, brush);
        }
    }

    void GradientRenderer::DrawRadialGradient(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (!m_renderTarget || !m_cache || stops.empty()) {
            return;
        }

        std::string key = GenerateGradientKey("radial_gradient", stops);
        auto* brush = m_cache->GetRadialGradient(key, center, radius, stops);
        if (brush) {
            D2D1_ELLIPSE ellipse = {};
            ellipse.point = ToD2DPoint(center);
            ellipse.radiusX = radius;
            ellipse.radiusY = radius;

            m_renderTarget->FillEllipse(&ellipse, brush);
        }
    }

    void GradientRenderer::DrawGradientCircle(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        bool filled
    ) {
        if (filled) {
            DrawRadialGradient(center, radius, stops);
        }
        else if (!stops.empty()) {
            const auto& backColor = stops.back().color;
            Color c = { backColor.r, backColor.g, backColor.b, backColor.a };

            if (m_solidBrush) {
                m_solidBrush->SetColor(ToD2DColor(c));

                float stroke = 2.0f;
                D2D1_ELLIPSE ellipse = {};
                ellipse.point = ToD2DPoint(center);
                ellipse.radiusX = radius - stroke / 2.0f;
                ellipse.radiusY = radius - stroke / 2.0f;

                m_renderTarget->DrawEllipse(&ellipse, m_solidBrush, stroke);
            }
        }
    }

    void GradientRenderer::DrawGradientPath(
        const std::vector<Point>& points,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float strokeWidth
    ) {
        if (!m_renderTarget || !m_cache || !m_geometryBuilder ||
            points.size() < 2 || stops.empty()) {
            return;
        }

        std::string key = GenerateGradientKey("path_gradient", stops);
        auto* brush = m_cache->GetLinearGradient(
            key,
            points.front(),
            points.back(),
            stops
        );

        auto geo = m_geometryBuilder->CreatePathFromPoints(points, false, false);

        if (brush && geo) {
            m_renderTarget->DrawGeometry(geo.Get(), brush, strokeWidth);
        }
    }

    void GradientRenderer::DrawAngularGradient(
        const Point& center,
        float radius,
        float startAngle,
        float endAngle,
        const Color& startColor,
        const Color& endColor
    ) {
        const int segments = 180;
        float sweep = endAngle - startAngle;

        if (fabsf(sweep) < 0.1f || !m_renderTarget || !m_geometryBuilder || !m_solidBrush) {
            return;
        }

        float angleStep = sweep / static_cast<float>(segments);

        for (int i = 0; i < segments; ++i) {
            float a0 = startAngle + i * angleStep;
            float a1 = startAngle + (i + 1) * angleStep;

            auto geo = m_geometryBuilder->CreateAngularSlice(center, radius, a0, a1);
            if (geo) {
                Color midColor = Utils::InterpolateColor(
                    startColor,
                    endColor,
                    (static_cast<float>(i) + 0.5f) / segments
                );

                m_solidBrush->SetColor(ToD2DColor(midColor));
                m_renderTarget->FillGeometry(geo.Get(), m_solidBrush);
            }
        }
    }

    void GradientRenderer::DrawVerticalGradientBar(
        const Rect& rect,
        const std::vector<D2D1_GRADIENT_STOP>& stops,
        float cornerRadius
    ) {
        if (!m_renderTarget || !m_cache || stops.empty()) {
            return;
        }

        std::string key = GenerateGradientKey("vbar_gradient", stops);
        auto* brush = m_cache->GetLinearGradient(
            key,
            { rect.x, rect.y },
            { rect.x, rect.GetBottom() },
            stops
        );

        if (!brush) {
            return;
        }

        if (cornerRadius > 0.0f) {
            D2D1_ROUNDED_RECT rr = {
                ToD2DRect(rect),
                cornerRadius,
                cornerRadius
            };
            m_renderTarget->FillRoundedRectangle(&rr, brush);
        }
        else {
            D2D1_RECT_F d2dRect = ToD2DRect(rect);
            m_renderTarget->FillRectangle(&d2dRect, brush);
        }
    }

    void GradientRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;
    }

}