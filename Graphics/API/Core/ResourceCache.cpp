// ResourceCache.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ResourceCache for Direct2D resource management.
//
// Implementation details:
// - Uses unordered_map for O(1) average lookup performance
// - Lazy creation pattern - resources created on first request
// - Automatic cache invalidation on render target change
// - Single lookup optimization (find once, insert if missing)
// - Uses D2DHelpers for validation, sanitization, and HRESULT checking
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ResourceCache.h"
#include "D2DHelpers.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::HResult;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ResourceCache::ResourceCache(ID2D1Factory* factory)
        : m_factory(factory), m_renderTarget(nullptr)
    {
    }

    void ResourceCache::OnRenderTargetChanged(ID2D1RenderTarget* renderTarget)
    {
        m_renderTarget = renderTarget;
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
    }

    void ResourceCache::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Retrieval
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ID2D1LinearGradientBrush* ResourceCache::GetLinearGradient(
        const std::string& key, const Point& start, const Point& end,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) const
    {
        auto it = m_linearGradientCache.find(key);
        if (it != m_linearGradientCache.end()) {
            return it->second.Get();
        }
        auto brush = CreateLinearGradient(start, end, stops);
        if (!brush) return nullptr;
        m_linearGradientCache[key] = brush;
        return brush.Get();
    }

    ID2D1RadialGradientBrush* ResourceCache::GetRadialGradient(
        const std::string& key, const Point& center, float radius,
        const std::vector<D2D1_GRADIENT_STOP>& stops
    ) const
    {
        auto it = m_radialGradientCache.find(key);
        if (it != m_radialGradientCache.end()) {
            return it->second.Get();
        }
        auto brush = CreateRadialGradient(center, radius, stops);
        if (!brush) return nullptr;
        m_radialGradientCache[key] = brush;
        return brush.Get();
    }

    ID2D1PathGeometry* ResourceCache::GetPathGeometry(
        const std::string& key, std::function<void(ID2D1GeometrySink*)> buildFunc
    ) const
    {
        auto it = m_geometryCache.find(key);
        if (it != m_geometryCache.end()) {
            return it->second.Get();
        }

        if (!m_factory) return nullptr;

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

        buildFunc(sink.Get());
        HRESULT closeHr = sink->Close();
        Check(closeHr, "ID2D1GeometrySink::Close");

        m_geometryCache[key] = geometry;
        return geometry.Get();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Cache Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ResourceCache::Clear()
    {
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
        m_geometryCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool ResourceCache::HasLinearGradient(const std::string& key) const noexcept
    {
        return m_linearGradientCache.find(key) != m_linearGradientCache.end();
    }

    bool ResourceCache::HasRadialGradient(const std::string& key) const noexcept
    {
        return m_radialGradientCache.find(key) != m_radialGradientCache.end();
    }

    bool ResourceCache::HasGeometry(const std::string& key) const noexcept
    {
        return m_geometryCache.find(key) != m_geometryCache.end();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1LinearGradientBrush> ResourceCache::CreateLinearGradient(
        const Point& start, const Point& end, const std::vector<D2D1_GRADIENT_STOP>& stops
    ) const
    {
        if (!m_renderTarget) return nullptr;
        if (!GradientStops(stops)) return nullptr;

        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        HRESULT hr = m_renderTarget->CreateGradientStopCollection(stops.data(), static_cast<UINT32>(stops.size()), stopCollection.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1RenderTarget::CreateGradientStopCollection", stopCollection)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1LinearGradientBrush> brush;
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = D2D1::LinearGradientBrushProperties(ToD2DPoint(start), ToD2DPoint(end));
        hr = m_renderTarget->CreateLinearGradientBrush(props, stopCollection.Get(), brush.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1RenderTarget::CreateLinearGradientBrush", brush)) {
            return nullptr;
        }
        return brush;
    }

    wrl::ComPtr<ID2D1RadialGradientBrush> ResourceCache::CreateRadialGradient(
        const Point& center, float radius, const std::vector<D2D1_GRADIENT_STOP>& stops
    ) const
    {
        if (!m_renderTarget) return nullptr;
        if (!GradientStops(stops)) return nullptr;
        if (!PositiveRadius(radius)) return nullptr;

        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        HRESULT hr = m_renderTarget->CreateGradientStopCollection(stops.data(), static_cast<UINT32>(stops.size()), stopCollection.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1RenderTarget::CreateGradientStopCollection", stopCollection)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1RadialGradientBrush> brush;
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props = D2D1::RadialGradientBrushProperties(ToD2DPoint(center), D2D1::Point2F(0.0f, 0.0f), radius, radius);
        hr = m_renderTarget->CreateRadialGradientBrush(props, stopCollection.Get(), brush.GetAddressOf());
        if (!CheckComCreation(hr, "ID2D1RenderTarget::CreateRadialGradientBrush", brush)) {
            return nullptr;
        }
        return brush;
    }

} // namespace Spectrum