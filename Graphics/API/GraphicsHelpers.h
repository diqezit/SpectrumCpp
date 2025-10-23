#ifndef SPECTRUM_GRAPHICS_HELPERS_H
#define SPECTRUM_GRAPHICS_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Unified Graphics Helpers - All inline utilities and helper functions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GraphicsAPI.h"
#include <cmath>
#include <algorithm>
#include <d2d1.h>
#include <dwrite.h>
#include <random>
#include <chrono>
#include <shared_mutex>
#include <optional>
#include <string_view>

namespace Spectrum::Helpers {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants - Centralized magic numbers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Constants {
        // Math epsilon for float comparisons
        constexpr float kEpsilon = 1e-6f;

        // Angular conversion factors
        constexpr float kDegToRad = PI / 180.0f;
        constexpr float kRadToDeg = 180.0f / PI;

        // Mel scale frequency conversion
        constexpr float kMelScale = 2595.0f;
        constexpr float kMelOffset = 700.0f;

        // Window size constraints
        constexpr int kMinWindowSize = 1;
        constexpr int kMaxWindowSize = 32767;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Type Conversion Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace TypeConversion {
        [[nodiscard]] inline D2D1_COLOR_F ToD2DColor(const Color& c) noexcept {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        [[nodiscard]] inline D2D1_POINT_2F ToD2DPoint(const Point& p) noexcept {
            return D2D1::Point2F(p.x, p.y);
        }

        [[nodiscard]] inline D2D1_RECT_F ToD2DRect(const Rect& r) noexcept {
            return D2D1::RectF(r.x, r.y, r.x + r.width, r.y + r.height);
        }

        [[nodiscard]] inline D2D1_SIZE_F ToD2DSize(float width, float height) noexcept {
            return D2D1::SizeF(width, height);
        }

        [[nodiscard]] inline D2D1_SIZE_U ToD2DSizeU(UINT32 width, UINT32 height) noexcept {
            return D2D1::SizeU(width, height);
        }

        [[nodiscard]] inline D2D1_ELLIPSE ToD2DEllipse(const Point& center, float radiusX, float radiusY) noexcept {
            return D2D1::Ellipse(ToD2DPoint(center), radiusX, radiusY);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Enum Conversion Helpers - Array-based for efficiency
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace EnumConversion {
        [[nodiscard]] inline D2D1_CAP_STYLE ToD2DCapStyle(StrokeCap cap) noexcept {
            constexpr D2D1_CAP_STYLE mapping[] = {
                D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_SQUARE
            };
            return mapping[static_cast<size_t>(cap)];
        }

        [[nodiscard]] inline D2D1_LINE_JOIN ToD2DLineJoin(StrokeJoin join) noexcept {
            constexpr D2D1_LINE_JOIN mapping[] = {
                D2D1_LINE_JOIN_MITER, D2D1_LINE_JOIN_ROUND, D2D1_LINE_JOIN_BEVEL
            };
            return mapping[static_cast<size_t>(join)];
        }

        [[nodiscard]] inline D2D1_DASH_STYLE ToD2DDashStyle(DashStyle style) noexcept {
            constexpr D2D1_DASH_STYLE mapping[] = {
                D2D1_DASH_STYLE_SOLID, D2D1_DASH_STYLE_DASH, D2D1_DASH_STYLE_DOT,
                D2D1_DASH_STYLE_DASH_DOT, D2D1_DASH_STYLE_DASH_DOT_DOT, D2D1_DASH_STYLE_CUSTOM
            };
            return mapping[static_cast<size_t>(style)];
        }

        [[nodiscard]] inline DWRITE_TEXT_ALIGNMENT ToDWriteTextAlign(TextAlign align) noexcept {
            constexpr DWRITE_TEXT_ALIGNMENT mapping[] = {
                DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_TEXT_ALIGNMENT_TRAILING,
                DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_TEXT_ALIGNMENT_JUSTIFIED
            };
            return mapping[static_cast<size_t>(align)];
        }

        [[nodiscard]] inline DWRITE_PARAGRAPH_ALIGNMENT ToDWriteParagraphAlign(ParagraphAlign align) noexcept {
            constexpr DWRITE_PARAGRAPH_ALIGNMENT mapping[] = {
                DWRITE_PARAGRAPH_ALIGNMENT_NEAR, DWRITE_PARAGRAPH_ALIGNMENT_FAR, DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            };
            return mapping[static_cast<size_t>(align)];
        }

        [[nodiscard]] inline DWRITE_FONT_WEIGHT ToDWriteFontWeight(FontWeight weight) noexcept {
            return static_cast<DWRITE_FONT_WEIGHT>(weight);
        }

        [[nodiscard]] inline DWRITE_FONT_STYLE ToDWriteFontStyle(FontStyle style) noexcept {
            constexpr DWRITE_FONT_STYLE mapping[] = {
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STYLE_ITALIC, DWRITE_FONT_STYLE_OBLIQUE
            };
            return mapping[static_cast<size_t>(style)];
        }

        [[nodiscard]] inline DWRITE_FONT_STRETCH ToDWriteFontStretch(FontStretch stretch) noexcept {
            return static_cast<DWRITE_FONT_STRETCH>(stretch);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Math Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Math {
        using Constants::kDegToRad;
        using Constants::kRadToDeg;
        using Constants::kEpsilon;

        template<typename T>
        [[nodiscard]] constexpr T Clamp(T value, T minVal, T maxVal) noexcept {
            return (value < minVal) ? minVal : (value > maxVal) ? maxVal : value;
        }

        template<typename T>
        [[nodiscard]] constexpr T Saturate(T value) noexcept {
            return Clamp(value, static_cast<T>(0), static_cast<T>(1));
        }

        template<typename T>
        [[nodiscard]] constexpr T Lerp(T a, T b, float t) noexcept {
            return a + (b - a) * t;
        }

        [[nodiscard]] inline constexpr float DegreesToRadians(float degrees) noexcept {
            return degrees * kDegToRad;
        }

        [[nodiscard]] inline constexpr float RadiansToDegrees(float radians) noexcept {
            return radians * kRadToDeg;
        }

        [[nodiscard]] inline float Normalize(float value, float minVal, float maxVal) noexcept {
            const float denom = maxVal - minVal;
            return (std::abs(denom) < kEpsilon) ? 0.0f : Clamp((value - minVal) / denom, 0.0f, 1.0f);
        }

        [[nodiscard]] inline float Map(float value, float inMin, float inMax, float outMin, float outMax) noexcept {
            const float normalized = Normalize(value, inMin, inMax);
            return outMin + normalized * (outMax - outMin);
        }

        [[nodiscard]] inline float FreqToMel(float freq) noexcept {
            return Constants::kMelScale * std::log10(1.0f + freq / Constants::kMelOffset);
        }

        [[nodiscard]] inline float MelToFreq(float mel) noexcept {
            return Constants::kMelOffset * (std::pow(10.0f, mel / Constants::kMelScale) - 1.0f);
        }

        [[nodiscard]] constexpr float EaseInQuad(float t) noexcept { return t * t; }
        [[nodiscard]] constexpr float EaseOutQuad(float t) noexcept { return t * (2.0f - t); }
        [[nodiscard]] constexpr float EaseInOutQuad(float t) noexcept {
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        }
        [[nodiscard]] constexpr float EaseInCubic(float t) noexcept { return t * t * t; }
        [[nodiscard]] constexpr float EaseOutCubic(float t) noexcept {
            const float f = t - 1.0f;
            return f * f * f + 1.0f;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation & Sanitization - Unified
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Validate {
        template<typename T>
        [[nodiscard]] inline bool Pointer(const T* ptr, const char* ptrName = nullptr, const char* componentName = nullptr) noexcept {
            if (!ptr) {
                if (componentName && ptrName) {
                    LOG_ERROR(componentName << ": " << ptrName << " is null");
                }
                return false;
            }
            (void)ptrName; (void)componentName;
            return true;
        }

        [[nodiscard]] inline bool Condition(bool condition, const char* message = nullptr, const char* componentName = nullptr) noexcept {
            if (!condition && componentName && message) {
                LOG_ERROR(componentName << ": " << message);
            }
            (void)message; (void)componentName;
            return condition;
        }

        template<typename T>
        [[nodiscard]] inline bool ArraySize(const std::vector<T>& array, size_t minSize) noexcept {
            return array.size() >= minSize;
        }

        template<typename T>
        [[nodiscard]] inline bool Positive(T value) noexcept {
            return value > static_cast<T>(0);
        }

        template<typename T>
        [[nodiscard]] inline bool Range(T value, T minVal, T maxVal) noexcept {
            return value >= minVal && value <= maxVal;
        }

        [[nodiscard]] inline bool NonZero(float value, float epsilon = Math::kEpsilon) noexcept {
            return std::abs(value) >= epsilon;
        }

        // Aliases for backward compatibility
        [[nodiscard]] inline bool PointArray(const std::vector<Point>& points, size_t minSize = 2) noexcept {
            return ArraySize(points, minSize);
        }

        [[nodiscard]] inline bool GradientStops(const std::vector<D2D1_GRADIENT_STOP>& stops) noexcept {
            return stops.size() >= 2;
        }

        [[nodiscard]] inline bool PositiveRadius(float radius) noexcept {
            return Positive(radius);
        }

        [[nodiscard]] inline bool RadiusRange(float innerRadius, float outerRadius) noexcept {
            return innerRadius >= 0.0f && innerRadius < outerRadius;
        }

        [[nodiscard]] inline bool NonZeroAngle(float angle) noexcept {
            return NonZero(angle, 0.01f);
        }
    }

    namespace Sanitize {
        template<typename T>
        [[nodiscard]] inline T Positive(T value, T defaultValue = T{}) noexcept {
            return (value > static_cast<T>(0)) ? value : defaultValue;
        }

        template<typename T>
        [[nodiscard]] inline T NonNegative(T value) noexcept {
            return std::max(value, static_cast<T>(0));
        }

        [[nodiscard]] inline float Normalized(float value) noexcept {
            if (std::isnan(value)) return 0.0f;
            if (std::isinf(value)) return value > 0.0f ? 1.0f : 0.0f;
            return Math::Clamp(value, 0.0f, 1.0f);
        }

        template<typename T>
        [[nodiscard]] inline T Clamped(T value, T minVal, T maxVal) noexcept {
            return Math::Clamp(value, minVal, maxVal);
        }

        // Aliases for backward compatibility
        [[nodiscard]] inline float PositiveFloat(float value, float defaultValue = 0.0f) noexcept {
            return Positive(value, defaultValue);
        }

        [[nodiscard]] inline float NonNegativeFloat(float value) noexcept {
            return NonNegative(value);
        }

        [[nodiscard]] inline float NormalizedFloat(float value) noexcept {
            return Normalized(value);
        }

        [[nodiscard]] inline int ClampValue(int value, int minValue, int maxValue) noexcept {
            return Math::Clamp(value, minValue, maxValue);
        }

        [[nodiscard]] inline int MinValue(int value, int minValue) noexcept {
            return std::max(value, minValue);
        }

        [[nodiscard]] inline float Radius(float value) noexcept {
            return Positive(value, 1.0f);
        }

        [[nodiscard]] inline int PolygonSides(int sides) noexcept {
            return std::max(sides, 3);
        }

        [[nodiscard]] inline int StarPoints(int points) noexcept {
            return std::max(points, 2);
        }

        [[nodiscard]] inline int CircleSegments(int segments) noexcept {
            return Math::Clamp(segments, 3, 360);
        }

        [[nodiscard]] inline bool PositiveRadius(float radius) noexcept {
            return Validate::Positive(radius);
        }

        [[nodiscard]] inline bool NonZeroAngle(float angle) noexcept {
            return Validate::NonZero(angle);
        }

        [[nodiscard]] inline bool PointArray(const std::vector<Point>& points, size_t minSize) noexcept {
            if (!Validate::ArraySize(points, minSize)) {
                LOG_WARNING("Point array too small: " << points.size() << " < " << minSize);
                return false;
            }
            return true;
        }
    }

    // Unified validation macro with variadic return value
#define VALIDATE_OR_RETURN(condition, ...) \
        do { if (!(condition)) return __VA_ARGS__; } while(0)

#define VALIDATE_PTR_OR_RETURN(ptr, component, ...) \
        VALIDATE_OR_RETURN(::Spectrum::Helpers::Validate::Pointer(ptr, #ptr, component), __VA_ARGS__)

#define VALIDATE_PTR_OR_RETURN_FALSE(ptr, component) \
        VALIDATE_PTR_OR_RETURN(ptr, component, false)

#define VALIDATE_PTR_OR_RETURN_NULL(ptr, component) \
        VALIDATE_PTR_OR_RETURN(ptr, component, nullptr)

#define VALIDATE_PTR_OR_RETURN_VALUE(ptr, component, value) \
        VALIDATE_PTR_OR_RETURN(ptr, component, value)

#define VALIDATE_CONDITION_OR_RETURN_FALSE(cond, msg, component) \
        VALIDATE_OR_RETURN(::Spectrum::Helpers::Validate::Condition(cond, msg, component), false)

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Geometry {
        // Point operations
        [[nodiscard]] constexpr Point Subtract(const Point& a, const Point& b) noexcept {
            return { a.x - b.x, a.y - b.y };
        }

        [[nodiscard]] constexpr Point Add(const Point& a, const Point& b) noexcept {
            return { a.x + b.x, a.y + b.y };
        }

        [[nodiscard]] constexpr Point Multiply(const Point& p, float scalar) noexcept {
            return { p.x * scalar, p.y * scalar };
        }

        [[nodiscard]] inline Point Divide(const Point& p, float scalar) noexcept {
            return (std::abs(scalar) < Math::kEpsilon)
                ? Point{ 0.0f, 0.0f }
            : Point{ p.x / scalar, p.y / scalar };
        }

        [[nodiscard]] inline float Length(const Point& p) noexcept {
            return std::sqrt(p.x * p.x + p.y * p.y);
        }

        [[nodiscard]] constexpr float DistanceSquared(const Point& a, const Point& b) noexcept {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            return dx * dx + dy * dy;
        }

        [[nodiscard]] inline float Distance(const Point& a, const Point& b) noexcept {
            return std::sqrt(DistanceSquared(a, b));
        }

        [[nodiscard]] inline Point Normalize(const Point& p) noexcept {
            const float len = Length(p);
            return (len < Math::kEpsilon) ? Point{ 0.0f, 0.0f } : Divide(p, len);
        }

        [[nodiscard]] inline Point PointOnCircle(const Point& center, float radius, float angleRadians) noexcept {
            return Add(center, Point{ radius * std::cos(angleRadians), radius * std::sin(angleRadians) });
        }

        [[nodiscard]] inline Point PointOnEllipse(const Point& center, float radiusX, float radiusY, float angleRadians) noexcept {
            return Add(center, Point{ radiusX * std::cos(angleRadians), radiusY * std::sin(angleRadians) });
        }

        [[nodiscard]] inline Point DirectionFromAngle(float angleRadians) noexcept {
            return { std::cos(angleRadians), std::sin(angleRadians) };
        }

        // Rectangle utilities
        [[nodiscard]] constexpr float GetRight(const Rect& rect) noexcept {
            return rect.x + rect.width;
        }

        [[nodiscard]] constexpr float GetBottom(const Rect& rect) noexcept {
            return rect.y + rect.height;
        }

        [[nodiscard]] constexpr Point GetCenter(const Rect& rect) noexcept {
            return { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
        }

        [[nodiscard]] constexpr Point GetTopLeft(const Rect& rect) noexcept {
            return { rect.x, rect.y };
        }

        [[nodiscard]] constexpr Point GetTopRight(const Rect& rect) noexcept {
            return { GetRight(rect), rect.y };
        }

        [[nodiscard]] constexpr Point GetBottomLeft(const Rect& rect) noexcept {
            return { rect.x, GetBottom(rect) };
        }

        [[nodiscard]] constexpr Point GetBottomRight(const Rect& rect) noexcept {
            return { GetRight(rect), GetBottom(rect) };
        }

        [[nodiscard]] constexpr bool Contains(const Rect& rect, const Point& point) noexcept {
            return point.x >= rect.x && point.x <= GetRight(rect) &&
                point.y >= rect.y && point.y <= GetBottom(rect);
        }

        [[nodiscard]] constexpr bool IsValid(const Rect& rect) noexcept {
            return rect.width > 0.0f && rect.height > 0.0f;
        }

        [[nodiscard]] constexpr Rect CreateCentered(const Point& center, float width, float height) noexcept {
            return { center.x - width * 0.5f, center.y - height * 0.5f, width, height };
        }

        [[nodiscard]] constexpr Rect CreateFromPoints(const Point& topLeft, const Point& bottomRight) noexcept {
            return { topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y };
        }

        [[nodiscard]] constexpr Rect Deflate(const Rect& rect, float amount) noexcept {
            return { rect.x + amount, rect.y + amount, rect.width - amount * 2.0f, rect.height - amount * 2.0f };
        }

        [[nodiscard]] constexpr Rect CreateViewportBounds(int width, int height) noexcept {
            return { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
        }

        [[nodiscard]] constexpr Point GetViewportCenter(int width, int height) noexcept {
            return { static_cast<float>(width) * 0.5f, static_cast<float>(height) * 0.5f };
        }

        [[nodiscard]] constexpr float GetMaxRadiusInViewport(int width, int height) noexcept {
            return std::min(width, height) * 0.5f;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace ColorHelpers {
        [[nodiscard]] constexpr uint8_t FloatToByte(float value) noexcept {
            return static_cast<uint8_t>(Math::Clamp(value * 255.0f + 0.5f, 0.0f, 255.0f));
        }

        [[nodiscard]] constexpr float ByteToFloat(uint8_t value) noexcept {
            return static_cast<float>(value) / 255.0f;
        }

        [[nodiscard]] constexpr uint32_t ColorToARGB(const ::Spectrum::Color& color) noexcept {
            return (static_cast<uint32_t>(FloatToByte(color.a)) << 24) |
                (static_cast<uint32_t>(FloatToByte(color.r)) << 16) |
                (static_cast<uint32_t>(FloatToByte(color.g)) << 8) |
                static_cast<uint32_t>(FloatToByte(color.b));
        }

        [[nodiscard]] constexpr ::Spectrum::Color ARGBtoColor(uint32_t argb) noexcept {
            return ::Spectrum::Color(
                ByteToFloat(static_cast<uint8_t>((argb >> 16) & 0xFF)),
                ByteToFloat(static_cast<uint8_t>((argb >> 8) & 0xFF)),
                ByteToFloat(static_cast<uint8_t>(argb & 0xFF)),
                ByteToFloat(static_cast<uint8_t>((argb >> 24) & 0xFF))
            );
        }

        [[nodiscard]] inline ::Spectrum::Color InterpolateColor(const ::Spectrum::Color& c1, const ::Spectrum::Color& c2, float t) noexcept {
            const float ct = Math::Saturate(t);
            return ::Spectrum::Color(
                Math::Lerp(c1.r, c2.r, ct),
                Math::Lerp(c1.g, c2.g, ct),
                Math::Lerp(c1.b, c2.b, ct),
                Math::Lerp(c1.a, c2.a, ct)
            );
        }

        [[nodiscard]] inline ::Spectrum::Color AdjustBrightness(const ::Spectrum::Color& color, float factor) noexcept {
            return ::Spectrum::Color(
                Math::Saturate(color.r * factor),
                Math::Saturate(color.g * factor),
                Math::Saturate(color.b * factor),
                color.a
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // GDI Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Gdi {
        struct DcDeleter {
            void operator()(HDC hdc) const noexcept {
                if (hdc) ::DeleteDC(hdc);
            }
        };

        struct BitmapDeleter {
            void operator()(HBITMAP bitmap) const noexcept {
                if (bitmap) ::DeleteObject(bitmap);
            }
        };

        using UniqueDc = std::unique_ptr<std::remove_pointer_t<HDC>, DcDeleter>;
        using UniqueBitmap = std::unique_ptr<std::remove_pointer_t<HBITMAP>, BitmapDeleter>;

        class ScopedSelectObject {
        public:
            ScopedSelectObject() noexcept : m_hdc(nullptr), m_oldObj(nullptr) {}

            ScopedSelectObject(HDC hdc, HGDIOBJ obj) noexcept
                : m_hdc(hdc), m_oldObj(obj ? ::SelectObject(hdc, obj) : nullptr) {
            }

            ~ScopedSelectObject() noexcept {
                Restore();
            }

            ScopedSelectObject(const ScopedSelectObject&) = delete;
            ScopedSelectObject& operator=(const ScopedSelectObject&) = delete;

            ScopedSelectObject(ScopedSelectObject&& other) noexcept
                : m_hdc(std::exchange(other.m_hdc, nullptr))
                , m_oldObj(std::exchange(other.m_oldObj, nullptr)) {
            }

            ScopedSelectObject& operator=(ScopedSelectObject&& other) noexcept {
                if (this != &other) {
                    Restore();
                    m_hdc = std::exchange(other.m_hdc, nullptr);
                    m_oldObj = std::exchange(other.m_oldObj, nullptr);
                }
                return *this;
            }

            [[nodiscard]] bool IsValid() const noexcept {
                return m_hdc && m_oldObj;
            }

        private:
            void Restore() noexcept {
                if (IsValid()) {
                    ::SelectObject(m_hdc, m_oldObj);
                }
            }

            HDC m_hdc;
            HGDIOBJ m_oldObj;
        };

        struct AlphaDC {
            UniqueDc dc;
            UniqueBitmap bitmap;
            ScopedSelectObject selection;

            AlphaDC() noexcept = default;

            AlphaDC(UniqueDc&& dc_, UniqueBitmap&& bitmap_, ScopedSelectObject&& selection_) noexcept
                : dc(std::move(dc_)), bitmap(std::move(bitmap_)), selection(std::move(selection_)) {
            }

            [[nodiscard]] bool IsValid() const noexcept {
                return dc && bitmap && selection.IsValid();
            }

            [[nodiscard]] HDC GetDC() const noexcept { return dc.get(); }
            [[nodiscard]] HBITMAP GetBitmap() const noexcept { return bitmap.get(); }

            void Reset() noexcept {
                selection = ScopedSelectObject();
                bitmap.reset();
                dc.reset();
            }
        };

        [[nodiscard]] inline UniqueDc CreateMemoryDC() noexcept {
            return UniqueDc(::CreateCompatibleDC(nullptr));
        }

        [[nodiscard]] inline UniqueBitmap CreateAlphaBitmap(HDC hdc, int width, int height, void** bits = nullptr) noexcept {
            if (!hdc || width <= 0 || height <= 0) return nullptr;

            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            return UniqueBitmap(::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, bits, nullptr, 0));
        }

        [[nodiscard]] inline AlphaDC CreateAlphaDC(int width, int height) noexcept {
            auto dc = CreateMemoryDC();
            if (!dc) return AlphaDC();

            auto bitmap = CreateAlphaBitmap(dc.get(), width, height);
            if (!bitmap) return AlphaDC();

            ScopedSelectObject selection(dc.get(), bitmap.get());
            if (!selection.IsValid()) return AlphaDC();

            return AlphaDC(std::move(dc), std::move(bitmap), std::move(selection));
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // D2D Scopes
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Scopes {
        class ScopedTransform final {
        public:
            ScopedTransform(ID2D1RenderTarget* renderTarget, const D2D1_MATRIX_3X2_F& transform)
                : m_renderTarget(renderTarget), m_oldTransform(D2D1::Matrix3x2F::Identity()) {
                if (m_renderTarget) {
                    m_renderTarget->GetTransform(&m_oldTransform);
                    m_renderTarget->SetTransform(transform * m_oldTransform);
                }
            }

            ~ScopedTransform() noexcept {
                if (m_renderTarget) {
                    m_renderTarget->SetTransform(m_oldTransform);
                }
            }

            ScopedTransform(const ScopedTransform&) = delete;
            ScopedTransform& operator=(const ScopedTransform&) = delete;

        private:
            ID2D1RenderTarget* m_renderTarget;
            D2D1_MATRIX_3X2_F m_oldTransform;
        };

        class ScopedOpacityLayer final {
        public:
            ScopedOpacityLayer(ID2D1RenderTarget* renderTarget, float opacity)
                : m_renderTarget(renderTarget), m_layerPushed(false) {
                if (!m_renderTarget) return;

                wrl::ComPtr<ID2D1Layer> layer;
                if (SUCCEEDED(m_renderTarget->CreateLayer(nullptr, &layer))) {
                    D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
                    params.opacity = Sanitize::NormalizedFloat(opacity);
                    m_renderTarget->PushLayer(&params, layer.Get());
                    m_layerPushed = true;
                }
                else {
                    LOG_WARNING("CreateLayer failed in ScopedOpacityLayer");
                }
            }

            ~ScopedOpacityLayer() noexcept {
                if (m_renderTarget && m_layerPushed) {
                    m_renderTarget->PopLayer();
                }
            }

            ScopedOpacityLayer(const ScopedOpacityLayer&) = delete;
            ScopedOpacityLayer& operator=(const ScopedOpacityLayer&) = delete;

        private:
            ID2D1RenderTarget* m_renderTarget;
            bool m_layerPushed;
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Rendering {
        class RenderValidation {
        public:
            [[nodiscard]] static constexpr bool ValidateRenderTarget(ID2D1RenderTarget* target) noexcept {
                return target != nullptr;
            }

            [[nodiscard]] static constexpr bool ValidateBrush(ID2D1Brush* brush) noexcept {
                return brush != nullptr;
            }

            [[nodiscard]] static bool ValidatePointArray(const std::vector<Point>& points, size_t minSize = 2) noexcept {
                return Validate::ArraySize(points, minSize);
            }

            [[nodiscard]] static bool ValidateTextRenderingContext(
                ID2D1RenderTarget* target,
                IDWriteFactory* factory,
                const std::wstring& text
            ) noexcept {
                return ValidateRenderTarget(target) && factory && !text.empty();
            }
        };

        class HashGenerator {
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

            [[nodiscard]] static size_t GenerateTextFormatKey(
                const std::wstring& fontFamily,
                float fontSize,
                DWRITE_FONT_WEIGHT weight,
                DWRITE_FONT_STYLE style,
                DWRITE_FONT_STRETCH stretch,
                DWRITE_TEXT_ALIGNMENT textAlign,
                DWRITE_PARAGRAPH_ALIGNMENT paragraphAlign
            ) noexcept {
                return GenerateHash(fontFamily, fontSize,
                    static_cast<size_t>(weight), static_cast<size_t>(style),
                    static_cast<size_t>(stretch), static_cast<size_t>(textAlign),
                    static_cast<size_t>(paragraphAlign));
            }
        };

        template<typename TKey, typename TResource>
        class RenderResourceCache {
        public:
            using ResourcePtr = wrl::ComPtr<TResource>;
            using CreateFunc = std::function<ResourcePtr()>;

            explicit RenderResourceCache(size_t maxSize = 1000) : m_maxSize(maxSize) {}

            [[nodiscard]] ResourcePtr GetOrCreate(const TKey& key, const CreateFunc& createFunc) {
                {
                    std::shared_lock lock(m_mutex);
                    if (auto it = m_cache.find(key); it != m_cache.end() && it->second) {
                        return it->second;
                    }
                }

                std::unique_lock lock(m_mutex);
                if (auto it = m_cache.find(key); it != m_cache.end() && it->second) {
                    return it->second;
                }

                auto resource = createFunc();
                if (resource) {
                    if (m_cache.size() >= m_maxSize && !m_cache.empty()) {
                        m_cache.erase(m_cache.begin());
                    }
                    m_cache[key] = resource;
                }
                return resource;
            }

            void Clear() {
                std::unique_lock lock(m_mutex);
                m_cache.clear();
            }

            [[nodiscard]] size_t Size() const noexcept {
                std::shared_lock lock(m_mutex);
                return m_cache.size();
            }

        private:
            mutable std::unordered_map<TKey, ResourcePtr> m_cache;
            mutable std::shared_mutex m_mutex;
            size_t m_maxSize;
        };

        class BrushManager {
        public:
            [[nodiscard]] static wrl::ComPtr<ID2D1SolidColorBrush> CreateSolidBrush(
                ID2D1RenderTarget* target, const ::Spectrum::Color& color) {
                if (!target) return nullptr;
                wrl::ComPtr<ID2D1SolidColorBrush> brush;
                return SUCCEEDED(target->CreateSolidColorBrush(TypeConversion::ToD2DColor(color), &brush))
                    ? brush : nullptr;
            }

            [[nodiscard]] static wrl::ComPtr<ID2D1LinearGradientBrush> CreateLinearGradientBrush(
                ID2D1RenderTarget* target, const Point& start, const Point& end, ID2D1GradientStopCollection* stops) {
                if (!target || !stops) return nullptr;
                wrl::ComPtr<ID2D1LinearGradientBrush> brush;
                return SUCCEEDED(target->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(TypeConversion::ToD2DPoint(start), TypeConversion::ToD2DPoint(end)),
                    stops, &brush)) ? brush : nullptr;
            }

            [[nodiscard]] static wrl::ComPtr<ID2D1RadialGradientBrush> CreateRadialGradientBrush(
                ID2D1RenderTarget* target, const Point& center, float radiusX, float radiusY, ID2D1GradientStopCollection* stops) {
                if (!target || !stops) return nullptr;
                wrl::ComPtr<ID2D1RadialGradientBrush> brush;
                return SUCCEEDED(target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(TypeConversion::ToD2DPoint(center), D2D1::Point2F(0, 0), radiusX, radiusY),
                    stops, &brush)) ? brush : nullptr;
            }

            [[nodiscard]] static wrl::ComPtr<ID2D1GradientStopCollection> CreateGradientStops(
                ID2D1RenderTarget* target, const std::vector<D2D1_GRADIENT_STOP>& stops) {
                if (!target || stops.empty()) return nullptr;
                wrl::ComPtr<ID2D1GradientStopCollection> collection;
                return SUCCEEDED(target->CreateGradientStopCollection(
                    stops.data(), static_cast<UINT32>(stops.size()),
                    D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &collection)) ? collection : nullptr;
            }
        };

        class FactoryHelper {
        public:
            [[nodiscard]] static wrl::ComPtr<IDWriteTextFormat> CreateTextFormat(
                IDWriteFactory* writeFactory,
                const std::wstring& fontFamily, float fontSize,
                DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL,
                const std::wstring& locale = L"en-us") {
                if (!writeFactory) return nullptr;
                wrl::ComPtr<IDWriteTextFormat> textFormat;
                return SUCCEEDED(writeFactory->CreateTextFormat(
                    fontFamily.c_str(), nullptr, weight, style, stretch, fontSize, locale.c_str(), &textFormat))
                    ? textFormat : nullptr;
            }
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Utils
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Utils {
        template<typename TEnum>
        [[nodiscard]] inline TEnum CycleEnum(TEnum current, int direction) {
            static_assert(std::is_enum_v<TEnum>);
            using TUnderlying = std::underlying_type_t<TEnum>;
            const auto count = static_cast<TUnderlying>(TEnum::Count);
            auto next = static_cast<TUnderlying>(current) + direction;
            return static_cast<TEnum>((next % count + count) % count);
        }

        inline std::string_view ToString(FFTWindowType type) {
            constexpr std::string_view names[] = { "Hann", "Hamming", "Blackman", "Rectangular" };
            return names[static_cast<size_t>(type)];
        }

        inline std::string_view ToString(SpectrumScale type) {
            constexpr std::string_view names[] = { "Linear", "Logarithmic", "Mel" };
            return names[static_cast<size_t>(type)];
        }

        class Timer {
        public:
            Timer() : m_startTime(std::chrono::steady_clock::now()) {}

            void Reset() noexcept {
                m_startTime = std::chrono::steady_clock::now();
            }

            [[nodiscard]] float GetElapsedSeconds() const noexcept {
                return std::chrono::duration<float>(std::chrono::steady_clock::now() - m_startTime).count();
            }

            [[nodiscard]] float GetElapsedMilliseconds() const noexcept {
                return GetElapsedSeconds() * 1000.0f;
            }

        private:
            std::chrono::steady_clock::time_point m_startTime;
        };

        class Random {
        public:
            static Random& Instance() {
                thread_local Random inst;
                return inst;
            }

            Random() : m_generator(std::random_device{}()), m_unitDist(0.0f, 1.0f) {}

            [[nodiscard]] float Float(float min = 0.0f, float max = 1.0f) {
                return min + m_unitDist(m_generator) * (max - min);
            }

            [[nodiscard]] int Int(int min, int max) {
                std::uniform_int_distribution<int> dist(min, max);
                return dist(m_generator);
            }

            [[nodiscard]] bool Bool(float probability = 0.5f) {
                return m_unitDist(m_generator) < Math::Saturate(probability);
            }

        private:
            std::mt19937 m_generator;
            std::uniform_real_distribution<float> m_unitDist;
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Window {
        using Constants::kMinWindowSize;
        using Constants::kMaxWindowSize;

        [[nodiscard]] inline bool IsWindowValid(HWND hwnd) noexcept {
            return hwnd && ::IsWindow(hwnd);
        }

        [[nodiscard]] inline bool IsValidSize(int width, int height) noexcept {
            return Validate::Range(width, kMinWindowSize, kMaxWindowSize) &&
                Validate::Range(height, kMinWindowSize, kMaxWindowSize);
        }

        struct WindowRect {
            int left, top, right, bottom;

            [[nodiscard]] constexpr int Width() const noexcept { return right - left; }
            [[nodiscard]] constexpr int Height() const noexcept { return bottom - top; }
            [[nodiscard]] constexpr bool IsValid() const noexcept { return Width() > 0 && Height() > 0; }
            [[nodiscard]] constexpr RECT ToRECT() const noexcept { return RECT{ left, top, right, bottom }; }

            [[nodiscard]] static constexpr WindowRect FromRECT(const RECT& rc) noexcept {
                return WindowRect{ rc.left, rc.top, rc.right, rc.bottom };
            }
        };

        [[nodiscard]] inline std::optional<WindowRect> GetClientRect(HWND hwnd) noexcept {
            if (!IsWindowValid(hwnd)) return std::nullopt;
            RECT rc;
            return ::GetClientRect(hwnd, &rc) ? std::make_optional(WindowRect::FromRECT(rc)) : std::nullopt;
        }

        inline bool HideWindow(HWND hwnd) noexcept {
            return IsWindowValid(hwnd) && ::ShowWindow(hwnd, SW_HIDE);
        }

        inline bool ShowWindowState(HWND hwnd, int cmdShow = SW_SHOW) noexcept {
            return IsWindowValid(hwnd) && ::ShowWindow(hwnd, cmdShow);
        }

        inline bool CenterWindow(HWND hwnd) noexcept {
            if (!IsWindowValid(hwnd)) return false;
            RECT rc;
            if (!::GetWindowRect(hwnd, &rc)) return false;
            const int x = (::GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
            const int y = (::GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
            return ::SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER) != 0;
        }

        inline bool PositionAtBottom(HWND hwnd, int height) noexcept {
            if (!IsWindowValid(hwnd)) return false;
            const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
            const int yPos = ::GetSystemMetrics(SM_CYSCREEN) - height;
            return ::SetWindowPos(hwnd, HWND_TOPMOST, 0, yPos, screenWidth, height, SWP_SHOWWINDOW) != 0;
        }

        [[nodiscard]] inline bool IsActiveAndVisible(HWND hwnd) noexcept {
            return IsWindowValid(hwnd) && ::IsWindowVisible(hwnd) && !::IsIconic(hwnd);
        }
    }

} // namespace Spectrum::Helpers

// Backward compatibility - selective imports to Spectrum namespace
namespace Spectrum {
    // Type conversions
    using Helpers::TypeConversion::ToD2DColor;
    using Helpers::TypeConversion::ToD2DPoint;
    using Helpers::TypeConversion::ToD2DRect;
    using Helpers::TypeConversion::ToD2DSize;
    using Helpers::TypeConversion::ToD2DSizeU;
    using Helpers::TypeConversion::ToD2DEllipse;

    // Enum conversions
    using Helpers::EnumConversion::ToD2DCapStyle;
    using Helpers::EnumConversion::ToD2DLineJoin;
    using Helpers::EnumConversion::ToD2DDashStyle;
    using Helpers::EnumConversion::ToDWriteTextAlign;
    using Helpers::EnumConversion::ToDWriteParagraphAlign;
    using Helpers::EnumConversion::ToDWriteFontWeight;
    using Helpers::EnumConversion::ToDWriteFontStyle;
    using Helpers::EnumConversion::ToDWriteFontStretch;

    // Math functions
    using Helpers::Math::Clamp;
    using Helpers::Math::Saturate;
    using Helpers::Math::Lerp;
    using Helpers::Math::Map;
    using Helpers::Math::Normalize;
    using Helpers::Math::DegreesToRadians;
    using Helpers::Math::RadiansToDegrees;

    // Geometry functions
    using Helpers::Geometry::Distance;
    using Helpers::Geometry::DistanceSquared;
    using Helpers::Geometry::Length;
    using Helpers::Geometry::PointOnCircle;

    // Color functions
    using Helpers::ColorHelpers::ColorToARGB;
    using Helpers::ColorHelpers::ARGBtoColor;
    using Helpers::ColorHelpers::InterpolateColor;
    using Helpers::ColorHelpers::AdjustBrightness;
    using Helpers::ColorHelpers::FloatToByte;
    using Helpers::ColorHelpers::ByteToFloat;
}

// Backward compatibility namespace alias for Helpers::Color::*
namespace Spectrum::Helpers {
    namespace Color = ColorHelpers;
}

#endif // SPECTRUM_GRAPHICS_HELPERS_H