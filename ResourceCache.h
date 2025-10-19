#ifndef SPECTRUM_CPP_RESOURCE_CACHE_H
#define SPECTRUM_CPP_RESOURCE_CACHE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the ResourceCache, a class for storing and reusing
// expensive D2D resources like gradient brushes and path geometries
// This avoids recreating them on every frame for better performance
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <map>
#include <string>
#include <functional>

namespace Spectrum {

    class ResourceCache {
    public:
        explicit ResourceCache(ID2D1Factory* factory, ID2D1RenderTarget* renderTarget);

        ID2D1LinearGradientBrush* GetLinearGradient(
            const std::string& key,
            const Point& start,
            const Point& end,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        );

        ID2D1RadialGradientBrush* GetRadialGradient(
            const std::string& key,
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        );

        ID2D1PathGeometry* GetPathGeometry(
            const std::string& key,
            std::function<void(ID2D1GeometrySink*)> buildFunc
        );

        void Clear();
        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        wrl::ComPtr<ID2D1LinearGradientBrush> CreateLinearGradient(
            const Point& start,
            const Point& end,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        );

        wrl::ComPtr<ID2D1RadialGradientBrush> CreateRadialGradient(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        );

        ID2D1Factory* m_factory;
        ID2D1RenderTarget* m_renderTarget;

        std::map<std::string, wrl::ComPtr<ID2D1LinearGradientBrush>> m_linearGradientCache;
        std::map<std::string, wrl::ComPtr<ID2D1RadialGradientBrush>> m_radialGradientCache;
        std::map<std::string, wrl::ComPtr<ID2D1PathGeometry>> m_geometryCache;
    };

} // namespace Spectrum

#endif