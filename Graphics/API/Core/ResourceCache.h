#ifndef SPECTRUM_CPP_RESOURCE_CACHE_H
#define SPECTRUM_CPP_RESOURCE_CACHE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the ResourceCache for managing expensive Direct2D resources.
//
// This class implements a caching layer for gradient brushes and path
// geometries, preventing redundant creation and improving rendering
// performance. Resources are stored with hash keys for optimal lookup.
//
// Key responsibilities:
// - Lazy creation and caching of brushes (solid, linear, radial)
// - Path geometry caching with custom builder functions
// - Automatic cache invalidation on render target change
// - Thread-safe access through shared_mutex with double-checked locking
//
// Design notes:
// - Get methods are const (use mutable cache maps for logical const)
// - Gradient brushes cached and reused with dynamic property updates
// - Alpha quantization reduces unique cache keys (0.05 step)
// - Color/position quantization in hash functions prevents float drift
// - Uses ComPtr for automatic reference counting
// - Methods split into focused units following SRP principle
//
// Performance characteristics:
// - O(1) average lookup time (unordered_map)
// - Resources created once and reused until eviction
// - Cache limits prevent unbounded memory growth
// - Device-dependent resources cleared on render target change
//
// Memory management:
// - Max 128 stop collections, 256 linear brushes, 256 radial brushes
// - LRU eviction when cache limits exceeded
// - All caches cleared on device lost or render target change
// - Quantized keys prevent cache fragmentation from floating-point drift
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <shared_mutex>

namespace Spectrum {

    class IBrush;
    class LinearGradientBrush;
    class RadialGradientBrush;
    struct GradientStop;

    class ResourceCache final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit ResourceCache(ID2D1Factory* factory);

        ResourceCache(const ResourceCache&) = delete;
        ResourceCache& operator=(const ResourceCache&) = delete;

        void OnRenderTargetChanged(
            const wrl::ComPtr<ID2D1RenderTarget>& renderTarget
        ) override;

        void OnDeviceLost() override;
        Priority GetPriority() const override { return Priority::High; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Retrieval
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1Brush> GetBrush(
            const std::shared_ptr<IBrush>& brushDef,
            float globalAlpha
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1PathGeometry> GetPathGeometry(
            const std::string& key,
            std::function<void(ID2D1GeometrySink*)> buildFunc
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Cache Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Clear();

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Types - Cache Keys
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct GradientStopsHash {
            size_t operator()(
                const std::vector<GradientStop>& stops
                ) const noexcept;
        };

        struct RadialBrushKey {
            size_t stopHash;
            float radiusX;
            float radiusY;
            float quantizedAlpha;

            bool operator==(const RadialBrushKey& other) const noexcept;
        };

        struct RadialBrushKeyHash {
            size_t operator()(const RadialBrushKey& key) const noexcept;
        };

        struct LinearBrushKey {
            size_t stopHash;
            float quantizedAlpha;

            bool operator==(const LinearBrushKey& other) const noexcept;
        };

        struct LinearBrushKeyHash {
            size_t operator()(const LinearBrushKey& key) const noexcept;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Brush Retrieval
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1SolidColorBrush> GetSolidColorBrush(
            const Color& color
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1LinearGradientBrush> GetLinearGradient(
            const LinearGradientBrush& brushDef,
            float globalAlpha
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1RadialGradientBrush> GetRadialGradient(
            const RadialGradientBrush& brushDef,
            float globalAlpha
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Stop Collection Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1GradientStopCollection>
            GetOrCreateStopCollection(
                const std::vector<GradientStop>& stops
            ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1GradientStopCollection>
            CreateGradientStopCollection(
                const std::vector<GradientStop>& stops
            ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1GradientStopCollection>
            TryGetStopCollectionFromCache(
                size_t hash
            ) const;

        void StoreStopCollectionInCache(
            size_t hash,
            const wrl::ComPtr<ID2D1GradientStopCollection>& collection
        ) const;

        [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> ConvertStops(
            const std::vector<GradientStop>& stops
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Brush Creation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1LinearGradientBrush>
            CreateLinearGradientBrush(
                const LinearGradientBrush& brushDef,
                ID2D1GradientStopCollection* stopCollection
            ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1RadialGradientBrush>
            CreateRadialGradientBrush(
                const RadialGradientBrush& brushDef,
                ID2D1GradientStopCollection* stopCollection
            ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Brush Cache Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1LinearGradientBrush>
            GetOrCreateLinearBrush(
                const LinearBrushKey& key,
                const LinearGradientBrush& brushDef,
                float globalAlpha
            ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1RadialGradientBrush>
            GetOrCreateRadialBrush(
                const RadialBrushKey& key,
                const RadialGradientBrush& brushDef,
                float globalAlpha
            ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1LinearGradientBrush>
            TryGetLinearBrushFromCache(
                const LinearBrushKey& key,
                const LinearGradientBrush& brushDef,
                float globalAlpha
            ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1RadialGradientBrush>
            TryGetRadialBrushFromCache(
                const RadialBrushKey& key,
                const RadialGradientBrush& brushDef,
                float globalAlpha
            ) const;

        void StoreLinearBrushInCache(
            const LinearBrushKey& key,
            const wrl::ComPtr<ID2D1LinearGradientBrush>& brush
        ) const;

        void StoreRadialBrushInCache(
            const RadialBrushKey& key,
            const wrl::ComPtr<ID2D1RadialGradientBrush>& brush
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Brush Property Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateLinearBrushProperties(
            ID2D1LinearGradientBrush* brush,
            const LinearGradientBrush& brushDef,
            float globalAlpha
        ) const;

        void UpdateRadialBrushProperties(
            ID2D1RadialGradientBrush* brush,
            const RadialGradientBrush& brushDef,
            float globalAlpha
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Cache Eviction
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        template<typename TKey, typename TValue, typename THash>
        void EvictOldestEntry(
            std::unordered_map<TKey, TValue, THash>& cache
        ) const;

        void EvictOldestLinearBrush() const;
        void EvictOldestRadialBrush() const;
        void EvictOldestStopCollection() const;
        void EvictOldestGeometry() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float QuantizeAlpha(float alpha) const noexcept;
        [[nodiscard]] float QuantizeFloat(float value, float step) const noexcept;

        [[nodiscard]] bool CanCreateGradient(
            const std::vector<GradientStop>& stops
        ) const noexcept;

        void ClearAllCaches();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1Factory* m_factory;
        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;

        mutable wrl::ComPtr<ID2D1SolidColorBrush> m_solidBrush;

        mutable std::unordered_map<
            size_t,
            wrl::ComPtr<ID2D1GradientStopCollection>
        > m_stopCollections;

        mutable std::unordered_map<
            std::string,
            wrl::ComPtr<ID2D1PathGeometry>
        > m_geometryCache;

        mutable std::unordered_map<
            LinearBrushKey,
            wrl::ComPtr<ID2D1LinearGradientBrush>,
            LinearBrushKeyHash
        > m_linearBrushCache;

        mutable std::unordered_map<
            RadialBrushKey,
            wrl::ComPtr<ID2D1RadialGradientBrush>,
            RadialBrushKeyHash
        > m_radialBrushCache;

        mutable std::shared_mutex m_mutex;

        static constexpr size_t MAX_STOP_COLLECTIONS = 128;
        static constexpr size_t MAX_LINEAR_BRUSHES = 256;
        static constexpr size_t MAX_RADIAL_BRUSHES = 256;
        static constexpr size_t MAX_GEOMETRIES = 64;
        static constexpr float ALPHA_QUANTIZATION = 0.05f;
        static constexpr float COLOR_QUANTIZATION = 1.0f / 255.0f;
        static constexpr float POSITION_QUANTIZATION = 0.01f;
        static constexpr float RADIUS_EPSILON = 0.01f;
        static constexpr float ALPHA_EPSILON = 0.001f;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RESOURCE_CACHE_H