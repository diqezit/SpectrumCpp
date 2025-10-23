#ifndef SPECTRUM_CPP_RENDER_HELPERS_H
#define SPECTRUM_CPP_RENDER_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines common rendering utilities shared across all renderer classes.
// Consolidates validation, caching, brush management, and other common
// rendering patterns to reduce code duplication.
//
// Key responsibilities:
// - Render target validation
// - Brush creation and management
// - Resource caching utilities
// - Hash generation for caching
// - State preservation helpers
//
// Design notes:
// - All functions are stateless or use const references
// - Template-based caching for type safety
// - Consistent error handling patterns
// - RAII helpers moved to D2DScopes.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Helpers/D2D/D2DScopes.h"
#include <functional>
#include <unordered_map>
#include <shared_mutex>

namespace Spectrum {
    namespace Helpers {
        namespace Rendering {

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Validation Utilities
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            class RenderValidation
            {
            public:
                // Render target validation
                [[nodiscard]] static bool ValidateRenderTarget(ID2D1RenderTarget* target) noexcept;
                [[nodiscard]] static bool ValidateWriteFactory(IDWriteFactory* factory) noexcept;
                [[nodiscard]] static bool ValidateBrush(ID2D1Brush* brush) noexcept;
                [[nodiscard]] static bool ValidateGeometry(ID2D1Geometry* geometry) noexcept;

                [[nodiscard]] static bool ValidateRenderingContext(
                    ID2D1RenderTarget* target,
                    ID2D1Brush* brush
                ) noexcept;

                [[nodiscard]] static bool ValidateTextRenderingContext(
                    ID2D1RenderTarget* target,
                    IDWriteFactory* factory,
                    const std::wstring& text
                ) noexcept;

                // Geometric validation (migrated from Validation.h)
                [[nodiscard]] static bool ValidatePointArray(
                    const std::vector<Point>& points,
                    size_t minSize = 2
                ) noexcept;

                [[nodiscard]] static bool ValidateGradientStops(
                    const std::vector<D2D1_GRADIENT_STOP>& stops
                ) noexcept;

                [[nodiscard]] static bool ValidatePositiveRadius(float radius) noexcept;

                [[nodiscard]] static bool ValidateRadiusRange(
                    float innerRadius,
                    float outerRadius
                ) noexcept;

                [[nodiscard]] static bool ValidateNonZeroAngle(float angle) noexcept;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Brush Management Utilities
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            class BrushManager
            {
            public:
                [[nodiscard]] static wrl::ComPtr<ID2D1SolidColorBrush> CreateSolidBrush(
                    ID2D1RenderTarget* target,
                    const Color& color
                );

                [[nodiscard]] static wrl::ComPtr<ID2D1LinearGradientBrush> CreateLinearGradientBrush(
                    ID2D1RenderTarget* target,
                    const Point& start,
                    const Point& end,
                    ID2D1GradientStopCollection* stops
                );

                [[nodiscard]] static wrl::ComPtr<ID2D1RadialGradientBrush> CreateRadialGradientBrush(
                    ID2D1RenderTarget* target,
                    const Point& center,
                    float radiusX,
                    float radiusY,
                    ID2D1GradientStopCollection* stops
                );

                [[nodiscard]] static wrl::ComPtr<ID2D1GradientStopCollection> CreateGradientStops(
                    ID2D1RenderTarget* target,
                    const std::vector<D2D1_GRADIENT_STOP>& stops
                );

                class BrushColorScope
                {
                public:
                    BrushColorScope(ID2D1SolidColorBrush* brush);
                    ~BrushColorScope();

                private:
                    ID2D1SolidColorBrush* m_brush;
                    D2D1_COLOR_F m_originalColor;
                };
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Resource Caching Utilities
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            template<typename TKey, typename TResource>
            class RenderResourceCache
            {
            public:
                using ResourcePtr = wrl::ComPtr<TResource>;
                using CreateFunc = std::function<ResourcePtr()>;

                [[nodiscard]] ResourcePtr GetOrCreate(
                    const TKey& key,
                    const CreateFunc& createFunc
                ) const {
                    {
                        std::shared_lock<std::shared_mutex> lock(m_mutex);
                        auto it = m_cache.find(key);
                        if (it != m_cache.end() && it->second) {
                            return it->second;
                        }
                    }

                    auto resource = createFunc();

                    if (resource) {
                        std::unique_lock<std::shared_mutex> lock(m_mutex);
                        auto it = m_cache.find(key);
                        if (it != m_cache.end() && it->second) {
                            return it->second;
                        }
                        m_cache[key] = resource;
                    }
                    return resource;
                }

                [[nodiscard]] ResourcePtr Get(const TKey& key) const {
                    std::shared_lock<std::shared_mutex> lock(m_mutex);
                    auto it = m_cache.find(key);
                    return (it != m_cache.end()) ? it->second : nullptr;
                }

                void Store(const TKey& key, ResourcePtr resource) const {
                    if (resource) {
                        std::unique_lock<std::shared_mutex> lock(m_mutex);
                        m_cache[key] = resource;
                    }
                }

                void Clear() {
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    m_cache.clear();
                }

                [[nodiscard]] size_t Size() const noexcept {
                    std::shared_lock<std::shared_mutex> lock(m_mutex);
                    return m_cache.size();
                }

            private:
                mutable std::unordered_map<TKey, ResourcePtr> m_cache;
                mutable std::shared_mutex m_mutex;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Hash Generation Utilities
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            class HashGenerator
            {
            public:
                static constexpr size_t kHashSeed = 0x9e3779b9;

                template<typename T>
                static void HashCombine(size_t& seed, const T& value) noexcept {
                    seed ^= std::hash<T>{}(value)+kHashSeed + (seed << 6) + (seed >> 2);
                }

                template<typename... Args>
                [[nodiscard]] static size_t GenerateHash(const Args&... args) noexcept {
                    size_t seed = 0;
                    (HashCombine(seed, args), ...);
                    return seed;
                }

                [[nodiscard]] static uint64_t GenerateStrokeStyleKey(
                    D2D1_CAP_STYLE startCap,
                    D2D1_CAP_STYLE endCap,
                    D2D1_CAP_STYLE dashCap,
                    D2D1_LINE_JOIN lineJoin,
                    D2D1_DASH_STYLE dashStyle,
                    float dashOffset
                ) noexcept;

                [[nodiscard]] static size_t GenerateTextFormatKey(
                    const std::wstring& fontFamily,
                    float fontSize,
                    DWRITE_FONT_WEIGHT weight,
                    DWRITE_FONT_STYLE style,
                    DWRITE_FONT_STRETCH stretch,
                    DWRITE_TEXT_ALIGNMENT textAlign,
                    DWRITE_PARAGRAPH_ALIGNMENT paragraphAlign
                ) noexcept;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Stroke Style Management
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            class StrokeStyleManager
            {
            public:
                [[nodiscard]] static wrl::ComPtr<ID2D1StrokeStyle> CreateStrokeStyle(
                    ID2D1Factory* factory,
                    const D2D1_STROKE_STYLE_PROPERTIES& properties,
                    const float* dashes = nullptr,
                    UINT32 dashCount = 0
                );

                [[nodiscard]] static D2D1_STROKE_STYLE_PROPERTIES CreateStrokeProperties(
                    D2D1_CAP_STYLE startCap,
                    D2D1_CAP_STYLE endCap,
                    D2D1_CAP_STYLE dashCap,
                    D2D1_LINE_JOIN lineJoin,
                    float miterLimit,
                    D2D1_DASH_STYLE dashStyle,
                    float dashOffset
                ) noexcept;
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Factory Utilities
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            class FactoryHelper
            {
            public:
                [[nodiscard]] static wrl::ComPtr<ID2D1Factory> GetFactoryFromRenderTarget(
                    ID2D1RenderTarget* renderTarget
                );

                [[nodiscard]] static wrl::ComPtr<IDWriteTextFormat> CreateTextFormat(
                    IDWriteFactory* writeFactory,
                    const std::wstring& fontFamily,
                    float fontSize,
                    DWRITE_FONT_WEIGHT weight,
                    DWRITE_FONT_STYLE style,
                    DWRITE_FONT_STRETCH stretch
                );

                [[nodiscard]] static wrl::ComPtr<IDWriteTextLayout> CreateTextLayout(
                    IDWriteFactory* writeFactory,
                    const std::wstring& text,
                    IDWriteTextFormat* format,
                    float maxWidth,
                    float maxHeight
                );
            };

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Common Rendering Patterns
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            class RenderPatterns
            {
            public:
                static void DrawWithShadow(
                    ID2D1RenderTarget* target,
                    const std::function<void()>& drawFunc,
                    const Point& shadowOffset,
                    const Color& shadowColor,
                    ID2D1SolidColorBrush* brush = nullptr
                );

                static void DrawWithOutline(
                    const std::function<void()>& drawFunc,
                    float outlineWidth,
                    int passes = 8
                );

                static void DrawMirrored(
                    ID2D1RenderTarget* target,
                    const std::function<void()>& drawFunc,
                    bool horizontal,
                    bool vertical,
                    const Point& pivot
                );
            };

        } // namespace Rendering
    } // namespace Helpers
} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDER_HELPERS_H