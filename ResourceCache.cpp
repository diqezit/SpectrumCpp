// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the ResourceCache class. It provides methods
// to get or create D2D resources, storing them in a map to prevent
// redundant creation, which improves rendering performance
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ResourceCache.h"
#include "StringUtils.h"

namespace Spectrum {

    namespace {
        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }
    }

    ResourceCache::ResourceCache(ID2D1Factory* factory, ID2D1RenderTarget* renderTarget)
        : m_factory(factory)
        , m_renderTarget(renderTarget)
    {
    }

    // provides a cached linear gradient brush
    // if not found, creates and stores it for future frames
    ID2D1LinearGradientBrush* ResourceCache::GetLinearGradient(
        const std::string& key,
        const Point& start,
        const Point& end,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (m_linearGradientCache.count(key)) {
            return m_linearGradientCache[key].Get();
        }

        auto brush = CreateLinearGradient(start, end, stops);
        if (brush) {
            m_linearGradientCache[key] = brush;
        }

        return brush.Get();
    }

    // provides a cached radial gradient brush
    // if not found, creates and stores it for future frames
    ID2D1RadialGradientBrush* ResourceCache::GetRadialGradient(
        const std::string& key,
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (m_radialGradientCache.count(key)) {
            return m_radialGradientCache[key].Get();
        }

        auto brush = CreateRadialGradient(center, radius, stops);
        if (brush) {
            m_radialGradientCache[key] = brush;
        }

        return brush.Get();
    }

    // provides cached path geometry
    // build function is only called if geometry is not in cache
    ID2D1PathGeometry* ResourceCache::GetPathGeometry(
        const std::string& key,
        std::function<void(ID2D1GeometrySink*)> buildFunc
    ) {
        if (m_geometryCache.count(key)) {
            return m_geometryCache[key].Get();
        }

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

        buildFunc(sink.Get());
        sink->Close();

        m_geometryCache[key] = geo;
        return geo.Get();
    }

    void ResourceCache::Clear() {
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
        m_geometryCache.clear();
    }

    // brushes are device-dependent so cache must be cleared when render target changes
    void ResourceCache::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;
        Clear();
    }

    wrl::ComPtr<ID2D1LinearGradientBrush> ResourceCache::CreateLinearGradient(
        const Point& start,
        const Point& end,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (!m_renderTarget) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        if (FAILED(m_renderTarget->CreateGradientStopCollection(
            stops.data(),
            static_cast<UINT32>(stops.size()),
            stopCollection.GetAddressOf()
        ))) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1LinearGradientBrush> brush;
        D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props =
            D2D1::LinearGradientBrushProperties(
                ToD2DPoint(start),
                ToD2DPoint(end)
            );

        if (FAILED(m_renderTarget->CreateLinearGradientBrush(
            props,
            stopCollection.Get(),
            brush.GetAddressOf()
        ))) {
            return nullptr;
        }

        return brush;
    }

    wrl::ComPtr<ID2D1RadialGradientBrush> ResourceCache::CreateRadialGradient(
        const Point& center,
        float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) {
        if (!m_renderTarget) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        if (FAILED(m_renderTarget->CreateGradientStopCollection(
            stops.data(),
            static_cast<UINT32>(stops.size()),
            stopCollection.GetAddressOf()
        ))) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1RadialGradientBrush> brush;
        D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props =
            D2D1::RadialGradientBrushProperties(
                ToD2DPoint(center),
                {},
                radius,
                radius
            );

        if (FAILED(m_renderTarget->CreateRadialGradientBrush(
            props,
            stopCollection.Get(),
            brush.GetAddressOf()
        ))) {
            return nullptr;
        }

        return brush;
    }

} // namespace Spectrum