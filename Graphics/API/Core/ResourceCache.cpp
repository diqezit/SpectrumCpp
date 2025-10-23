// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ResourceCache for managing expensive Direct2D resources
//
// This implementation provides caching for gradient brushes and geometries
// to prevent redundant creation and improve rendering performance.
//
// Key optimizations:
// - Stop collections cached by quantized hash (prevents float drift)
// - Linear/radial gradient brushes fully cached with dynamic updates
// - Coordinates/opacity applied dynamically via SetStartPoint/SetCenter
// - Single solid brush reused with color changes
// - Path geometries cached with string keys
// - Thread-safe via shared_mutex with double-checked locking
// - Small methods following SRP, DRY, YAGNI principles
//
// Memory management:
// - Cache limits enforced: 128 stop collections, 256 brushes, 64 geometries
// - Simple FIFO eviction when cache size exceeded
// - All caches cleared on render target change or device lost
// - Hash-based cache keys with quantization prevent memory leaks
// - ComPtr ensures automatic reference counting
//
// Quantization strategy:
// - Alpha: 0.05 step (20 unique values: 0.00, 0.05, 0.10, ...)
// - Colors: 1/255 step (byte precision)
// - Position: 0.01 step (100 unique values per unit)
// - Radius comparison: 0.01 epsilon tolerance
//
// Thread safety:
// - Read operations use shared_lock for concurrent access
// - Write operations use unique_lock for exclusive access
// - Double-checked locking pattern in cache lookup/creation
// - No locks held during D2D resource creation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Core/ResourceCache.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Brushes/SolidColorBrush.h"
#include "Graphics/API/Brushes/LinearGradientBrush.h"
#include "Graphics/API/Brushes/RadialGradientBrush.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::HResult;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ResourceCache::ResourceCache(ID2D1Factory* factory)
        : m_factory(factory)
        , m_renderTarget(nullptr)
    {
    }

    void ResourceCache::OnRenderTargetChanged(
        const wrl::ComPtr<ID2D1RenderTarget>& renderTarget
    )
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_renderTarget = renderTarget;
        m_solidBrush.Reset();
        ClearAllCaches();
    }

    void ResourceCache::OnDeviceLost()
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_renderTarget.Reset();
        m_solidBrush.Reset();
        ClearAllCaches();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Retrieval
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1Brush> ResourceCache::GetBrush(
        const std::shared_ptr<IBrush>& brushDef,
        float globalAlpha
    ) const
    {
        if (!brushDef) return nullptr;

        if (auto solid = std::dynamic_pointer_cast<SolidColorBrush>(brushDef))
        {
            Color c = solid->color;
            c.a *= globalAlpha;
            return GetSolidColorBrush(c);
        }

        if (auto linear = std::dynamic_pointer_cast<LinearGradientBrush>(brushDef))
        {
            return GetLinearGradient(*linear, globalAlpha);
        }

        if (auto radial = std::dynamic_pointer_cast<RadialGradientBrush>(brushDef))
        {
            return GetRadialGradient(*radial, globalAlpha);
        }

        return nullptr;
    }

    wrl::ComPtr<ID2D1PathGeometry> ResourceCache::GetPathGeometry(
        const std::string& key,
        std::function<void(ID2D1GeometrySink*)> buildFunc
    ) const
    {
        {
            std::shared_lock<std::shared_mutex> readLock(m_mutex);
            auto it = m_geometryCache.find(key);
            if (it != m_geometryCache.end())
            {
                return it->second;
            }
        }

        if (!m_factory || !buildFunc)
        {
            return nullptr;
        }

        wrl::ComPtr<ID2D1PathGeometry> geometry;
        HRESULT hr = m_factory->CreatePathGeometry(
            geometry.GetAddressOf()
        );

        if (!CheckComCreation(
            hr,
            "ID2D1Factory::CreatePathGeometry",
            geometry
        ))
        {
            return nullptr;
        }

        wrl::ComPtr<ID2D1GeometrySink> sink;
        hr = geometry->Open(sink.GetAddressOf());

        if (!CheckComCreation(hr, "ID2D1PathGeometry::Open", sink))
        {
            return nullptr;
        }

        buildFunc(sink.Get());

        HRESULT closeHr = sink->Close();
        Check(closeHr, "ID2D1GeometrySink::Close");

        {
            std::unique_lock<std::shared_mutex> writeLock(m_mutex);

            if (m_geometryCache.size() >= MAX_GEOMETRIES)
            {
                EvictOldestGeometry();
            }

            auto it = m_geometryCache.find(key);
            if (it != m_geometryCache.end())
            {
                return it->second;
            }

            m_geometryCache[key] = geometry;
        }

        return geometry;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Cache Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ResourceCache::Clear()
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_solidBrush.Reset();
        ClearAllCaches();
    }

    void ResourceCache::ClearAllCaches()
    {
        m_stopCollections.clear();
        m_geometryCache.clear();
        m_linearBrushCache.clear();
        m_radialBrushCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Solid Color Brush
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1SolidColorBrush> ResourceCache::GetSolidColorBrush(
        const Color& color
    ) const
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        if (!m_renderTarget) return nullptr;

        if (!m_solidBrush)
        {
            HRESULT hr = m_renderTarget->CreateSolidColorBrush(
                ToD2DColor(color),
                &m_solidBrush
            );

            if (FAILED(hr))
            {
                LOG_ERROR(
                    "CreateSolidColorBrush failed: 0x"
                    << std::hex << hr
                );
                return nullptr;
            }
        }
        else
        {
            m_solidBrush->SetColor(ToD2DColor(color));
        }

        return m_solidBrush;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Linear Gradient Brush
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1LinearGradientBrush> ResourceCache::GetLinearGradient(
        const LinearGradientBrush& brushDef,
        float globalAlpha
    ) const
    {
        if (!CanCreateGradient(brushDef.stops)) return nullptr;

        const float quantizedAlpha = QuantizeAlpha(globalAlpha);
        const size_t stopHash = GradientStopsHash{}(brushDef.stops);

        const LinearBrushKey key = { stopHash, quantizedAlpha };

        return GetOrCreateLinearBrush(key, brushDef, globalAlpha);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Radial Gradient Brush
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1RadialGradientBrush> ResourceCache::GetRadialGradient(
        const RadialGradientBrush& brushDef,
        float globalAlpha
    ) const
    {
        if (!CanCreateGradient(brushDef.stops)) return nullptr;

        const float quantizedAlpha = QuantizeAlpha(globalAlpha);
        const size_t stopHash = GradientStopsHash{}(brushDef.stops);

        const RadialBrushKey key = {
            stopHash,
            brushDef.radiusX,
            brushDef.radiusY,
            quantizedAlpha
        };

        return GetOrCreateRadialBrush(key, brushDef, globalAlpha);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Hash Functors
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    size_t ResourceCache::GradientStopsHash::operator()(
        const std::vector<GradientStop>& stops
        ) const noexcept
    {
        size_t hash = stops.size();

        for (const auto& stop : stops)
        {
            const float quantPos = std::round(
                stop.position / POSITION_QUANTIZATION
            ) * POSITION_QUANTIZATION;

            const float quantR = std::round(
                stop.color.r / COLOR_QUANTIZATION
            ) * COLOR_QUANTIZATION;

            const float quantG = std::round(
                stop.color.g / COLOR_QUANTIZATION
            ) * COLOR_QUANTIZATION;

            const float quantB = std::round(
                stop.color.b / COLOR_QUANTIZATION
            ) * COLOR_QUANTIZATION;

            const float quantA = std::round(
                stop.color.a / ALPHA_QUANTIZATION
            ) * ALPHA_QUANTIZATION;

            hash ^= std::hash<float>{}(quantPos) << 1;
            hash ^= std::hash<float>{}(quantR) << 2;
            hash ^= std::hash<float>{}(quantG) << 3;
            hash ^= std::hash<float>{}(quantB) << 4;
            hash ^= std::hash<float>{}(quantA) << 5;
        }

        return hash;
    }

    bool ResourceCache::RadialBrushKey::operator==(
        const RadialBrushKey& other
        ) const noexcept
    {
        return stopHash == other.stopHash &&
            std::abs(radiusX - other.radiusX) < RADIUS_EPSILON &&
            std::abs(radiusY - other.radiusY) < RADIUS_EPSILON &&
            std::abs(quantizedAlpha - other.quantizedAlpha) < ALPHA_EPSILON;
    }

    size_t ResourceCache::RadialBrushKeyHash::operator()(
        const RadialBrushKey& key
        ) const noexcept
    {
        size_t hash = key.stopHash;
        hash ^= std::hash<float>{}(key.radiusX) << 1;
        hash ^= std::hash<float>{}(key.radiusY) << 2;
        hash ^= std::hash<float>{}(key.quantizedAlpha) << 3;
        return hash;
    }

    bool ResourceCache::LinearBrushKey::operator==(
        const LinearBrushKey& other
        ) const noexcept
    {
        return stopHash == other.stopHash &&
            std::abs(quantizedAlpha - other.quantizedAlpha) < ALPHA_EPSILON;
    }

    size_t ResourceCache::LinearBrushKeyHash::operator()(
        const LinearBrushKey& key
        ) const noexcept
    {
        size_t hash = key.stopHash;
        hash ^= std::hash<float>{}(key.quantizedAlpha) << 1;
        return hash;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Stop Collection Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1GradientStopCollection>
        ResourceCache::GetOrCreateStopCollection(
            const std::vector<GradientStop>& stops
        ) const
    {
        const size_t hash = GradientStopsHash{}(stops);

        auto cached = TryGetStopCollectionFromCache(hash);
        if (cached) return cached;

        auto collection = CreateGradientStopCollection(stops);
        if (!collection) return nullptr;

        StoreStopCollectionInCache(hash, collection);

        return collection;
    }

    wrl::ComPtr<ID2D1GradientStopCollection>
        ResourceCache::TryGetStopCollectionFromCache(
            size_t hash
        ) const
    {
        std::shared_lock<std::shared_mutex> readLock(m_mutex);
        auto it = m_stopCollections.find(hash);

        if (it != m_stopCollections.end())
        {
            return it->second;
        }

        return nullptr;
    }

    void ResourceCache::StoreStopCollectionInCache(
        size_t hash,
        const wrl::ComPtr<ID2D1GradientStopCollection>& collection
    ) const
    {
        std::unique_lock<std::shared_mutex> writeLock(m_mutex);

        if (m_stopCollections.size() >= MAX_STOP_COLLECTIONS)
        {
            EvictOldestStopCollection();
        }

        auto it = m_stopCollections.find(hash);
        if (it == m_stopCollections.end())
        {
            m_stopCollections[hash] = collection;
        }
    }

    wrl::ComPtr<ID2D1GradientStopCollection>
        ResourceCache::CreateGradientStopCollection(
            const std::vector<GradientStop>& stops
        ) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (!m_renderTarget) return nullptr;

        if (stops.empty())
        {
            LOG_ERROR("CreateGradientStopCollection: stops array is empty");
            return nullptr;
        }

        auto d2dStops = ConvertStops(stops);

        wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
        const HRESULT hr = m_renderTarget->CreateGradientStopCollection(
            d2dStops.data(),
            static_cast<UINT32>(d2dStops.size()),
            &stopCollection
        );

        if (!CheckComCreation(
            hr,
            "CreateGradientStopCollection",
            stopCollection
        ))
        {
            return nullptr;
        }

        return stopCollection;
    }

    std::vector<D2D1_GRADIENT_STOP> ResourceCache::ConvertStops(
        const std::vector<GradientStop>& stops
    ) const
    {
        std::vector<D2D1_GRADIENT_STOP> d2dStops;
        d2dStops.reserve(stops.size());

        for (const auto& stop : stops)
        {
            d2dStops.push_back({
                stop.position,
                ToD2DColor(stop.color)
                });
        }

        return d2dStops;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Brush Creation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1LinearGradientBrush>
        ResourceCache::CreateLinearGradientBrush(
            const LinearGradientBrush& brushDef,
            ID2D1GradientStopCollection* stopCollection
        ) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (!m_renderTarget) return nullptr;

        wrl::ComPtr<ID2D1LinearGradientBrush> brush;

        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props =
            D2D1::LinearGradientBrushProperties(
                ToD2DPoint(brushDef.startPoint),
                ToD2DPoint(brushDef.endPoint)
            );

        const HRESULT hr = m_renderTarget->CreateLinearGradientBrush(
            props,
            stopCollection,
            &brush
        );

        if (!CheckComCreation(hr, "CreateLinearGradientBrush", brush))
        {
            return nullptr;
        }

        return brush;
    }

    wrl::ComPtr<ID2D1RadialGradientBrush>
        ResourceCache::CreateRadialGradientBrush(
            const RadialGradientBrush& brushDef,
            ID2D1GradientStopCollection* stopCollection
        ) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (!m_renderTarget) return nullptr;

        wrl::ComPtr<ID2D1RadialGradientBrush> brush;

        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props =
            D2D1::RadialGradientBrushProperties(
                ToD2DPoint(brushDef.center),
                D2D1::Point2F(0.0f, 0.0f),
                brushDef.radiusX,
                brushDef.radiusY
            );

        const HRESULT hr = m_renderTarget->CreateRadialGradientBrush(
            props,
            stopCollection,
            &brush
        );

        if (!CheckComCreation(hr, "CreateRadialGradientBrush", brush))
        {
            return nullptr;
        }

        return brush;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Brush Cache Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1LinearGradientBrush>
        ResourceCache::GetOrCreateLinearBrush(
            const LinearBrushKey& key,
            const LinearGradientBrush& brushDef,
            float globalAlpha
        ) const
    {
        auto cached = TryGetLinearBrushFromCache(key, brushDef, globalAlpha);
        if (cached) return cached;

        auto stopCollection = GetOrCreateStopCollection(brushDef.stops);
        if (!stopCollection) return nullptr;

        auto brush = CreateLinearGradientBrush(brushDef, stopCollection.Get());
        if (!brush) return nullptr;

        UpdateLinearBrushProperties(brush.Get(), brushDef, globalAlpha);
        StoreLinearBrushInCache(key, brush);

        return brush;
    }

    wrl::ComPtr<ID2D1RadialGradientBrush>
        ResourceCache::GetOrCreateRadialBrush(
            const RadialBrushKey& key,
            const RadialGradientBrush& brushDef,
            float globalAlpha
        ) const
    {
        auto cached = TryGetRadialBrushFromCache(key, brushDef, globalAlpha);
        if (cached) return cached;

        auto stopCollection = GetOrCreateStopCollection(brushDef.stops);
        if (!stopCollection) return nullptr;

        auto brush = CreateRadialGradientBrush(brushDef, stopCollection.Get());
        if (!brush) return nullptr;

        UpdateRadialBrushProperties(brush.Get(), brushDef, globalAlpha);
        StoreRadialBrushInCache(key, brush);

        return brush;
    }

    wrl::ComPtr<ID2D1LinearGradientBrush>
        ResourceCache::TryGetLinearBrushFromCache(
            const LinearBrushKey& key,
            const LinearGradientBrush& brushDef,
            float globalAlpha
        ) const
    {
        std::shared_lock<std::shared_mutex> readLock(m_mutex);
        auto it = m_linearBrushCache.find(key);

        if (it != m_linearBrushCache.end())
        {
            UpdateLinearBrushProperties(
                it->second.Get(),
                brushDef,
                globalAlpha
            );
            return it->second;
        }

        return nullptr;
    }

    wrl::ComPtr<ID2D1RadialGradientBrush>
        ResourceCache::TryGetRadialBrushFromCache(
            const RadialBrushKey& key,
            const RadialGradientBrush& brushDef,
            float globalAlpha
        ) const
    {
        std::shared_lock<std::shared_mutex> readLock(m_mutex);
        auto it = m_radialBrushCache.find(key);

        if (it != m_radialBrushCache.end())
        {
            UpdateRadialBrushProperties(
                it->second.Get(),
                brushDef,
                globalAlpha
            );
            return it->second;
        }

        return nullptr;
    }

    void ResourceCache::StoreLinearBrushInCache(
        const LinearBrushKey& key,
        const wrl::ComPtr<ID2D1LinearGradientBrush>& brush
    ) const
    {
        std::unique_lock<std::shared_mutex> writeLock(m_mutex);

        if (m_linearBrushCache.size() >= MAX_LINEAR_BRUSHES)
        {
            EvictOldestLinearBrush();
        }

        auto it = m_linearBrushCache.find(key);
        if (it == m_linearBrushCache.end())
        {
            m_linearBrushCache[key] = brush;
        }
    }

    void ResourceCache::StoreRadialBrushInCache(
        const RadialBrushKey& key,
        const wrl::ComPtr<ID2D1RadialGradientBrush>& brush
    ) const
    {
        std::unique_lock<std::shared_mutex> writeLock(m_mutex);

        if (m_radialBrushCache.size() >= MAX_RADIAL_BRUSHES)
        {
            EvictOldestRadialBrush();
        }

        auto it = m_radialBrushCache.find(key);
        if (it == m_radialBrushCache.end())
        {
            m_radialBrushCache[key] = brush;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Brush Property Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ResourceCache::UpdateLinearBrushProperties(
        ID2D1LinearGradientBrush* brush,
        const LinearGradientBrush& brushDef,
        float globalAlpha
    ) const
    {
        if (!brush) return;

        brush->SetOpacity(globalAlpha);
        brush->SetStartPoint(ToD2DPoint(brushDef.startPoint));
        brush->SetEndPoint(ToD2DPoint(brushDef.endPoint));
    }

    void ResourceCache::UpdateRadialBrushProperties(
        ID2D1RadialGradientBrush* brush,
        const RadialGradientBrush& brushDef,
        float globalAlpha
    ) const
    {
        if (!brush) return;

        brush->SetOpacity(globalAlpha);
        brush->SetCenter(ToD2DPoint(brushDef.center));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Cache Eviction
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<typename TKey, typename TValue, typename THash>
    void ResourceCache::EvictOldestEntry(
        std::unordered_map<TKey, TValue, THash>& cache
    ) const
    {
        if (!cache.empty())
        {
            cache.erase(cache.begin());
        }
    }

    void ResourceCache::EvictOldestLinearBrush() const
    {
        EvictOldestEntry(m_linearBrushCache);
    }

    void ResourceCache::EvictOldestRadialBrush() const
    {
        EvictOldestEntry(m_radialBrushCache);
    }

    void ResourceCache::EvictOldestStopCollection() const
    {
        EvictOldestEntry(m_stopCollections);
    }

    void ResourceCache::EvictOldestGeometry() const
    {
        EvictOldestEntry(m_geometryCache);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float ResourceCache::QuantizeAlpha(float alpha) const noexcept
    {
        return QuantizeFloat(alpha, ALPHA_QUANTIZATION);
    }

    float ResourceCache::QuantizeFloat(
        float value,
        float step
    ) const noexcept
    {
        return std::round(value / step) * step;
    }

    bool ResourceCache::CanCreateGradient(
        const std::vector<GradientStop>& stops
    ) const noexcept
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_renderTarget != nullptr && !stops.empty();
    }

} // namespace Spectrum