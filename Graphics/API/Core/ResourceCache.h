// ResourceCache.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the ResourceCache for managing expensive Direct2D resources.
//
// This class implements a caching layer for gradient brushes and path
// geometries, preventing redundant creation and improving rendering
// performance. Resources are stored with string keys for flexible lookup.
//
// Key responsibilities:
// - Lazy creation of gradient brushes (linear and radial)
// - Path geometry caching with custom builder functions
// - Automatic cache invalidation on render target change
//
// Design notes:
// - Get methods are const (use mutable cache maps for logical const)
// - Single lookup pattern for optimal performance
// - Non-owning pointers to D2D resources (lifetime managed externally)
//
// Performance characteristics:
// - O(1) average lookup time (unordered_map)
// - Resources created once and reused until cache clear
// - Device-dependent resources cleared on render target change
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_RESOURCE_CACHE_H
#define SPECTRUM_CPP_RESOURCE_CACHE_H

#include "Common.h"
#include "Core/IRenderComponent.h"
#include <unordered_map>
#include <string>
#include <functional>

namespace Spectrum {

    class ResourceCache final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit ResourceCache(ID2D1Factory* factory);

        ResourceCache(const ResourceCache&) = delete;
        ResourceCache& operator=(const ResourceCache&) = delete;

        // IRenderComponent implementation
        void OnRenderTargetChanged(ID2D1RenderTarget* renderTarget) override;
        void OnDeviceLost() override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Retrieval
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] ID2D1LinearGradientBrush* GetLinearGradient(
            const std::string& key,
            const Point& start,
            const Point& end,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) const;

        [[nodiscard]] ID2D1RadialGradientBrush* GetRadialGradient(
            const std::string& key,
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) const;

        [[nodiscard]] ID2D1PathGeometry* GetPathGeometry(
            const std::string& key,
            std::function<void(ID2D1GeometrySink*)> buildFunc
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Cache Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Clear();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HasLinearGradient(const std::string& key) const noexcept;
        [[nodiscard]] bool HasRadialGradient(const std::string& key) const noexcept;
        [[nodiscard]] bool HasGeometry(const std::string& key) const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1LinearGradientBrush> CreateLinearGradient(
            const Point& start,
            const Point& end,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1RadialGradientBrush> CreateRadialGradient(
            const Point& center,
            float radius,
            const std::vector<D2D1_GRADIENT_STOP>& stops
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1Factory* m_factory;
        ID2D1RenderTarget* m_renderTarget;

        mutable std::unordered_map<std::string, wrl::ComPtr<ID2D1LinearGradientBrush>> m_linearGradientCache;
        mutable std::unordered_map<std::string, wrl::ComPtr<ID2D1RadialGradientBrush>> m_radialGradientCache;
        mutable std::unordered_map<std::string, wrl::ComPtr<ID2D1PathGeometry>> m_geometryCache;
    };

} // namespace Spectrum

#endif