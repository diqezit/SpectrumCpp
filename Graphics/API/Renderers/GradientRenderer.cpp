// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the GradientRenderer class. This file handles creation and
// rendering of gradient brushes with various configurations.
//
// Implementation details:
// - Gradient stops converted from custom format to D2D format
// - Angle-based linear gradients calculated from rect bounds
// - Brush creation delegated to RenderHelpers for consistency
// - Uses ResourceCache for gradient brush caching
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Renderers/GradientRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"
#include "Graphics/API/Core/ResourceCache.h"
#include "Graphics/API/Brushes/GradientStop.h"
#include <cmath>

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Rendering;

    namespace {
        constexpr float kDegToRad = 3.14159265f / 180.0f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GradientRenderer::GradientRenderer(Spectrum::ResourceCache* resourceCache)
        : m_resourceCache(resourceCache)
    {
    }

    void GradientRenderer::OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget)
    {
        m_renderTarget = renderTarget;
    }

    void GradientRenderer::OnDeviceLost()
    {
        m_renderTarget.Reset();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradient Stop Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    std::vector<D2D1_GRADIENT_STOP> GradientRenderer::ConvertGradientStops(
        const std::vector<GradientStop>& stops
    ) const {
        std::vector<D2D1_GRADIENT_STOP> d2dStops;
        d2dStops.reserve(stops.size());

        for (const auto& stop : stops) {
            d2dStops.push_back({
                stop.position,
                ToD2DColor(stop.color)
                });
        }

        return d2dStops;
    }

    wrl::ComPtr<ID2D1GradientStopCollection> GradientRenderer::CreateStopCollection(
        const std::vector<D2D1_GRADIENT_STOP>& d2dStops
    ) const {
        if (!m_renderTarget || d2dStops.empty()) {
            return nullptr;
        }

        return BrushManager::CreateGradientStops(m_renderTarget.Get(), d2dStops);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradient Point Calculation (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Point GradientRenderer::CalculateGradientStart(
        const Rect& rect,
        float angle
    ) const noexcept {
        const float radians = angle * kDegToRad;
        const float centerX = rect.x + rect.width * 0.5f;
        const float centerY = rect.y + rect.height * 0.5f;
        const float radius = std::sqrt(rect.width * rect.width + rect.height * rect.height) * 0.5f;

        return {
            centerX - radius * std::cos(radians),
            centerY - radius * std::sin(radians)
        };
    }

    Point GradientRenderer::CalculateGradientEnd(
        const Rect& rect,
        float angle
    ) const noexcept {
        const float radians = angle * kDegToRad;
        const float centerX = rect.x + rect.width * 0.5f;
        const float centerY = rect.y + rect.height * 0.5f;
        const float radius = std::sqrt(rect.width * rect.width + rect.height * rect.height) * 0.5f;

        return {
            centerX + radius * std::cos(radians),
            centerY + radius * std::sin(radians)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Brush Creation Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1LinearGradientBrush> GradientRenderer::CreateLinearBrush(
        const Point& start,
        const Point& end,
        ID2D1GradientStopCollection* stops
    ) const {
        return BrushManager::CreateLinearGradientBrush(
            m_renderTarget.Get(),
            start,
            end,
            stops
        );
    }

    wrl::ComPtr<ID2D1RadialGradientBrush> GradientRenderer::CreateRadialBrush(
        const Point& center,
        const Point& origin,
        float radiusX,
        float radiusY,
        ID2D1GradientStopCollection* stops
    ) const {
        if (!m_renderTarget || !stops) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1RadialGradientBrush> brush;
        HRESULT hr = m_renderTarget->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(
                ToD2DPoint(center),
                ToD2DPoint(origin),
                radiusX,
                radiusY
            ),
            stops,
            &brush
        );

        if (FAILED(hr)) {
            return nullptr;
        }

        return brush;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Gradient Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GradientRenderer::DrawLinearGradient(
        const Rect& rect,
        const Point& startPoint,
        const Point& endPoint,
        const std::vector<GradientStop>& stops
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (stops.empty()) return;

        auto d2dStops = ConvertGradientStops(stops);
        auto stopCollection = CreateStopCollection(d2dStops);
        if (!stopCollection) return;

        auto brush = CreateLinearBrush(startPoint, endPoint, stopCollection.Get());
        if (!brush) return;

        m_renderTarget->FillRectangle(ToD2DRect(rect), brush.Get());
    }

    void GradientRenderer::DrawRadialGradient(
        const Point& center,
        float radiusX,
        float radiusY,
        const std::vector<GradientStop>& stops,
        const Point& gradientOrigin
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (stops.empty()) return;

        auto d2dStops = ConvertGradientStops(stops);
        auto stopCollection = CreateStopCollection(d2dStops);
        if (!stopCollection) return;

        auto brush = CreateRadialBrush(center, gradientOrigin, radiusX, radiusY, stopCollection.Get());
        if (!brush) return;

        const D2D1_ELLIPSE ellipse = {
            ToD2DPoint(center),
            radiusX,
            radiusY
        };

        m_renderTarget->FillEllipse(&ellipse, brush.Get());
    }

    void GradientRenderer::FillRectWithLinearGradient(
        const Rect& rect,
        const std::vector<GradientStop>& stops,
        float angle
    ) const {
        const Point start = CalculateGradientStart(rect, angle);
        const Point end = CalculateGradientEnd(rect, angle);

        DrawLinearGradient(rect, start, end, stops);
    }

    void GradientRenderer::FillCircleWithRadialGradient(
        const Point& center,
        float radius,
        const std::vector<GradientStop>& stops
    ) const {
        DrawRadialGradient(center, radius, radius, stops);
    }

} // namespace Spectrum