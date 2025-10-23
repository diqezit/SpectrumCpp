#include "Graphics/API/Core/ResourceCache.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Brushes/SolidColorBrush.h"
#include "Graphics/API/Brushes/LinearGradientBrush.h"
#include "Graphics/API/Brushes/RadialGradientBrush.h"
#include <sstream>
#include <iomanip>

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::HResult;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ResourceCache::ResourceCache(
        ID2D1Factory* factory
    )
        : m_factory(factory), m_renderTarget(nullptr)
    {
    }

    void ResourceCache::OnRenderTargetChanged(
        ID2D1RenderTarget* renderTarget
    )
    {
        m_renderTarget = renderTarget;
        m_solidBrush.Reset();
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
        m_geometryCache.clear();
    }

    void ResourceCache::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        m_solidBrush.Reset();
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
        m_geometryCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Retrieval
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1Brush> ResourceCache::GetBrush(
        const std::shared_ptr<IBrush>& brushDef,
        float globalAlpha
    ) const
    {
        if (!brushDef) {
            return nullptr;
        }

        if (auto solid = std::dynamic_pointer_cast<SolidColorBrush>(brushDef)) {
            Color c = solid->color;
            c.a *= globalAlpha;
            return GetSolidColorBrush(c);
        }

        if (auto linear = std::dynamic_pointer_cast<LinearGradientBrush>(brushDef)) {
            auto brush = GetLinearGradient(*linear);
            if (brush) {
                brush->SetOpacity(globalAlpha);
            }
            return brush;
        }

        if (auto radial = std::dynamic_pointer_cast<RadialGradientBrush>(brushDef)) {
            auto brush = GetRadialGradient(*radial);
            if (brush) {
                brush->SetOpacity(globalAlpha);
            }
            return brush;
        }

        return nullptr;
    }

    ID2D1PathGeometry* ResourceCache::GetPathGeometry(
        const std::string& key,
        std::function<void(ID2D1GeometrySink*)> buildFunc
    ) const
    {
        auto it = m_geometryCache.find(key);
        if (it != m_geometryCache.end()) {
            return it->second.Get();
        }

        if (!m_factory) {
            return nullptr;
        }

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
        m_solidBrush.Reset();
        m_linearGradientCache.clear();
        m_radialGradientCache.clear();
        m_geometryCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1SolidColorBrush> ResourceCache::GetSolidColorBrush(
        const Color& color
    ) const {
        if (!m_renderTarget) {
            return nullptr;
        }

        if (!m_solidBrush) {
            m_renderTarget->CreateSolidColorBrush(
                ToD2DColor(color),
                &m_solidBrush
            );
        }
        else {
            m_solidBrush->SetColor(ToD2DColor(color));
        }
        return m_solidBrush;
    }

    wrl::ComPtr<ID2D1LinearGradientBrush> ResourceCache::GetLinearGradient(
        const LinearGradientBrush& brushDef
    ) const
    {
        const std::string key = GenerateKey(brushDef);
        auto it = m_linearGradientCache.find(key);
        if (it != m_linearGradientCache.end()) {
            return it->second;
        }

        if (!m_renderTarget || brushDef.stops.empty()) {
            return nullptr;
        }

        auto d2dStops = ConvertStops(brushDef.stops);
        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        HRESULT hr = m_renderTarget->CreateGradientStopCollection(
            d2dStops.data(),
            static_cast<UINT32>(d2dStops.size()),
            &stopCollection
        );
        if (!CheckComCreation(hr, "CreateGradientStopCollection", stopCollection)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1LinearGradientBrush> brush;
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = D2D1::LinearGradientBrushProperties(
            ToD2DPoint(brushDef.startPoint),
            ToD2DPoint(brushDef.endPoint)
        );
        hr = m_renderTarget->CreateLinearGradientBrush(
            props,
            stopCollection.Get(),
            &brush
        );
        if (!CheckComCreation(hr, "CreateLinearGradientBrush", brush)) {
            return nullptr;
        }

        m_linearGradientCache[key] = brush;
        return brush;
    }

    wrl::ComPtr<ID2D1RadialGradientBrush> ResourceCache::GetRadialGradient(
        const RadialGradientBrush& brushDef
    ) const
    {
        const std::string key = GenerateKey(brushDef);
        auto it = m_radialGradientCache.find(key);
        if (it != m_radialGradientCache.end()) {
            return it->second;
        }

        if (!m_renderTarget || brushDef.stops.empty()) {
            return nullptr;
        }

        auto d2dStops = ConvertStops(brushDef.stops);
        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        HRESULT hr = m_renderTarget->CreateGradientStopCollection(
            d2dStops.data(),
            static_cast<UINT32>(d2dStops.size()),
            &stopCollection
        );
        if (!CheckComCreation(hr, "CreateGradientStopCollection", stopCollection)) {
            return nullptr;
        }

        wrl::ComPtr<ID2D1RadialGradientBrush> brush;
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props = D2D1::RadialGradientBrushProperties(
            ToD2DPoint(brushDef.center),
            { 0,0 },
            brushDef.radiusX,
            brushDef.radiusY
        );
        hr = m_renderTarget->CreateRadialGradientBrush(
            props,
            stopCollection.Get(),
            &brush
        );
        if (!CheckComCreation(hr, "CreateRadialGradientBrush", brush)) {
            return nullptr;
        }

        m_radialGradientCache[key] = brush;
        return brush;
    }

    std::string ResourceCache::GenerateKey(
        const LinearGradientBrush& brushDef
    ) const {
        std::ostringstream oss;
        oss << "lin_" << std::fixed << std::setprecision(2)
            << brushDef.startPoint.x << "_" << brushDef.startPoint.y << "_"
            << brushDef.endPoint.x << "_" << brushDef.endPoint.y;
        for (const auto& stop : brushDef.stops) {
            oss << "_" << stop.position << "_" << stop.color.r << stop.color.g << stop.color.b << stop.color.a;
        }
        return oss.str();
    }

    std::string ResourceCache::GenerateKey(
        const RadialGradientBrush& brushDef
    ) const {
        std::ostringstream oss;
        oss << "rad_" << std::fixed << std::setprecision(2)
            << brushDef.center.x << "_" << brushDef.center.y << "_"
            << brushDef.radiusX << "_" << brushDef.radiusY;
        for (const auto& stop : brushDef.stops) {
            oss << "_" << stop.position << "_" << stop.color.r << stop.color.g << stop.color.b << stop.color.a;
        }
        return oss.str();
    }

    std::vector<D2D1_GRADIENT_STOP> ResourceCache::ConvertStops(
        const std::vector<GradientStop>& stops
    ) const {
        std::vector<D2D1_GRADIENT_STOP> d2dStops;
        d2dStops.reserve(stops.size());
        for (const auto& stop : stops) {
            d2dStops.push_back({ stop.position, ToD2DColor(stop.color) });
        }
        return d2dStops;
    }

} // namespace Spectrum