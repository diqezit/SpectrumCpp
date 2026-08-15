#ifndef SPECTRUM_GRAPHICS_HELPERS_H
#define SPECTRUM_GRAPHICS_HELPERS_H

#include "Common/Common.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Spectrum::Helpers {

    namespace Constants {
        constexpr float kEpsilon = 1e-6f;
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kTwoPi = 6.28318530717958647692f;
        constexpr float kDegToRad = kPi / 180.0f;
        constexpr float kRadToDeg = 180.0f / kPi;
        constexpr float kMelScale = 2595.0f;
        constexpr float kMelOffset = 700.0f;
        constexpr int   kMinWindowSize = 1;
        constexpr int   kMaxWindowSize = 32767;
    }

    namespace Math {
        using Constants::kDegToRad;
        using Constants::kRadToDeg;
        using Constants::kEpsilon;

        template<typename T>
        constexpr T Clamp(T v, T lo, T hi) noexcept {
            return v < lo ? lo : v > hi ? hi : v;
        }

        template<typename T>
        constexpr T Saturate(T v) noexcept { return Clamp(v, T(0), T(1)); }

        template<typename T>
        constexpr T Lerp(T a, T b, float t) noexcept { return a + (b - a) * t; }

        constexpr float DegreesToRadians(float d) noexcept { return d * kDegToRad; }
        constexpr float RadiansToDegrees(float r) noexcept { return r * kRadToDeg; }

        inline float Normalize(float v, float lo, float hi) noexcept {
            return Clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
        }

        inline float Map(float v, float inLo, float inHi, float outLo, float outHi) noexcept {
            return outLo + Normalize(v, inLo, inHi) * (outHi - outLo);
        }

        inline float FreqToMel(float f) noexcept {
            return Constants::kMelScale * std::log10(1.0f + f / Constants::kMelOffset);
        }

        inline float MelToFreq(float m) noexcept {
            return Constants::kMelOffset * (std::pow(10.0f, m / Constants::kMelScale) - 1.0f);
        }

        constexpr float EaseInQuad(float t) noexcept { return t * t; }
        constexpr float EaseOutQuad(float t) noexcept { return t * (2.0f - t); }

        constexpr float EaseInOutQuad(float t) noexcept {
            return t < 0.5f
                ? 2.0f * t * t
                : -1.0f + (4.0f - 2.0f * t) * t;
        }

        constexpr float EaseInCubic(float t) noexcept { return t * t * t; }

        constexpr float EaseOutCubic(float t) noexcept {
            const float f = t - 1.0f;
            return f * f * f + 1.0f;
        }

        constexpr float SmoothStep(float e0, float e1, float x) noexcept {
            const float t = Saturate((x - e0) / (e1 - e0));
            return t * t * (3.0f - 2.0f * t);
        }
    }

    namespace Validate {
        template<typename T>
        inline bool Pointer(const T* p, const char* = nullptr, const char* = nullptr) noexcept {
            return p != nullptr;
        }

        inline bool Condition(bool ok, const char* = nullptr, const char* = nullptr) noexcept {
            return ok;
        }

        template<typename T>
        inline bool ArraySize(const std::vector<T>& a, size_t n) noexcept { return a.size() >= n; }

        template<typename T>
        inline bool Positive(T v) noexcept { return v > T(0); }

        template<typename T>
        inline bool Range(T v, T lo, T hi) noexcept { return v >= lo && v <= hi; }

        inline bool NonZero(float v, float eps = Math::kEpsilon) noexcept { return std::abs(v) >= eps; }
        inline bool PointArray(const std::vector<Point>& p, size_t n = 2) noexcept { return ArraySize(p, n); }
        inline bool PositiveRadius(float r) noexcept { return r > 0.0f; }
        inline bool RadiusRange(float inner, float outer) noexcept { return inner >= 0.0f && inner < outer; }
        inline bool NonZeroAngle(float a) noexcept { return NonZero(a, 0.01f); }
    }

    namespace Sanitize {
        template<typename T>
        inline T Positive(T v, T def = T{}) noexcept { return v > T(0) ? v : def; }

        template<typename T>
        inline T NonNegative(T v) noexcept { return std::max(v, T(0)); }

        template<typename T>
        inline T Clamped(T v, T lo, T hi) noexcept { return Math::Clamp(v, lo, hi); }

        inline float Normalized(float v) noexcept { return Math::Clamp(v, 0.0f, 1.0f); }
        inline float Radius(float v) noexcept { return Positive(v, 1.0f); }
        inline int PolygonSides(int n) noexcept { return std::max(n, 3); }
        inline int StarPoints(int n) noexcept { return std::max(n, 2); }
        inline int CircleSegments(int n) noexcept { return Math::Clamp(n, 3, 360); }
        inline bool PointArray(const std::vector<Point>& p, size_t n) noexcept { return Validate::ArraySize(p, n); }
    }

#define VALIDATE_OR_RETURN(condition, ...) \
    do { if (!(condition)) return __VA_ARGS__; } while (0)

#define VALIDATE_PTR_OR_RETURN(ptr, component, ...) \
    VALIDATE_OR_RETURN(::Spectrum::Helpers::Validate::Pointer(ptr, #ptr, component), __VA_ARGS__)

#define VALIDATE_PTR_OR_RETURN_FALSE(ptr, component) \
    VALIDATE_PTR_OR_RETURN(ptr, component, false)

#define VALIDATE_PTR_OR_RETURN_NULL(ptr, component) \
    VALIDATE_PTR_OR_RETURN(ptr, component, nullptr)

#define VALIDATE_CONDITION_OR_RETURN_FALSE(cond, msg, component) \
    VALIDATE_OR_RETURN(::Spectrum::Helpers::Validate::Condition(cond, msg, component), false)

    namespace Geometry {
        constexpr Point Add(const Point& a, const Point& b) noexcept { return { a.x + b.x, a.y + b.y }; }
        constexpr Point Subtract(const Point& a, const Point& b) noexcept { return { a.x - b.x, a.y - b.y }; }
        constexpr Point Multiply(const Point& p, float s) noexcept { return { p.x * s, p.y * s }; }
        inline Point Divide(const Point& p, float s) noexcept { return { p.x / s, p.y / s }; }

        inline float Length(const Point& p) noexcept { return std::sqrt(p.x * p.x + p.y * p.y); }

        constexpr float DistanceSquared(const Point& a, const Point& b) noexcept {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            return dx * dx + dy * dy;
        }

        inline float Distance(const Point& a, const Point& b) noexcept {
            return std::sqrt(DistanceSquared(a, b));
        }

        inline Point Normalize(const Point& p) noexcept { return Divide(p, Length(p)); }

        inline Point PointOnCircle(const Point& c, float r, float a) noexcept {
            return Add(c, { r * std::cos(a), r * std::sin(a) });
        }

        inline Point PointOnEllipse(const Point& c, float rx, float ry, float a) noexcept {
            return Add(c, { rx * std::cos(a), ry * std::sin(a) });
        }

        inline Point DirectionFromAngle(float a) noexcept { return { std::cos(a), std::sin(a) }; }

        constexpr float GetRight(const Rect& r) noexcept { return r.x + r.width; }
        constexpr float GetBottom(const Rect& r) noexcept { return r.y + r.height; }
        constexpr Point GetCenter(const Rect& r) noexcept { return { r.x + r.width * 0.5f, r.y + r.height * 0.5f }; }
        constexpr Point GetTopLeft(const Rect& r) noexcept { return { r.x, r.y }; }
        constexpr Point GetTopRight(const Rect& r) noexcept { return { GetRight(r), r.y }; }
        constexpr Point GetBottomLeft(const Rect& r) noexcept { return { r.x, GetBottom(r) }; }
        constexpr Point GetBottomRight(const Rect& r) noexcept { return { GetRight(r), GetBottom(r) }; }

        constexpr bool Contains(const Rect& r, const Point& p) noexcept {
            return p.x >= r.x && p.x <= GetRight(r)
                && p.y >= r.y && p.y <= GetBottom(r);
        }

        constexpr bool IsValid(const Rect& r) noexcept { return r.width > 0.0f && r.height > 0.0f; }

        constexpr Rect CreateCentered(const Point& c, float w, float h) noexcept {
            return { c.x - w * 0.5f, c.y - h * 0.5f, w, h };
        }

        constexpr Rect CreateFromPoints(const Point& tl, const Point& br) noexcept {
            return { tl.x, tl.y, br.x - tl.x, br.y - tl.y };
        }

        constexpr Rect Deflate(const Rect& r, float a) noexcept {
            return { r.x + a, r.y + a, r.width - a * 2.0f, r.height - a * 2.0f };
        }

        constexpr Rect CreateViewportBounds(int w, int h) noexcept {
            return { 0.0f, 0.0f, float(w), float(h) };
        }

        constexpr Point GetViewportCenter(int w, int h) noexcept {
            return { float(w) * 0.5f, float(h) * 0.5f };
        }

        inline float GetMaxRadiusInViewport(int w, int h) noexcept {
            return float(std::min(w, h)) * 0.5f;
        }
    }

    namespace ColorHelpers {
        constexpr uint8_t FloatToByte(float v) noexcept {
            return uint8_t(Math::Clamp(v * 255.0f + 0.5f, 0.0f, 255.0f));
        }

        constexpr float ByteToFloat(uint8_t v) noexcept { return float(v) / 255.0f; }

        constexpr uint32_t ColorToARGB(const ::Spectrum::Color& c) noexcept {
            return (uint32_t(FloatToByte(c.a)) << 24)
                | (uint32_t(FloatToByte(c.r)) << 16)
                | (uint32_t(FloatToByte(c.g)) << 8)
                | uint32_t(FloatToByte(c.b));
        }

        constexpr ::Spectrum::Color ARGBtoColor(uint32_t argb) noexcept {
            return ::Spectrum::Color(
                ByteToFloat(uint8_t((argb >> 16) & 0xFF)),
                ByteToFloat(uint8_t((argb >> 8) & 0xFF)),
                ByteToFloat(uint8_t(argb & 0xFF)),
                ByteToFloat(uint8_t((argb >> 24) & 0xFF)));
        }

        inline ::Spectrum::Color InterpolateColor(
            const ::Spectrum::Color& a,
            const ::Spectrum::Color& b,
            float t) noexcept
        {
            t = Math::Saturate(t);
            return ::Spectrum::Color(
                Math::Lerp(a.r, b.r, t),
                Math::Lerp(a.g, b.g, t),
                Math::Lerp(a.b, b.b, t),
                Math::Lerp(a.a, b.a, t));
        }

        inline ::Spectrum::Color AdjustBrightness(const ::Spectrum::Color& c, float f) noexcept {
            return ::Spectrum::Color(
                Math::Saturate(c.r * f),
                Math::Saturate(c.g * f),
                Math::Saturate(c.b * f),
                c.a);
        }

        inline ::Spectrum::Color AdjustSaturation(const ::Spectrum::Color& c, float f) noexcept {
            const float gray = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
            return ::Spectrum::Color(
                Math::Lerp(gray, c.r, f),
                Math::Lerp(gray, c.g, f),
                Math::Lerp(gray, c.b, f),
                c.a);
        }

        inline ::Spectrum::Color AdjustAlpha(const ::Spectrum::Color& c, float a) noexcept {
            return ::Spectrum::Color(c.r, c.g, c.b, Math::Saturate(a));
        }
    }

    namespace Gdi {
        struct DcDeleter {
            void operator()(HDC h) const noexcept { ::DeleteDC(h); }
        };

        struct BitmapDeleter {
            void operator()(HBITMAP h) const noexcept { ::DeleteObject(h); }
        };

        using UniqueDc = std::unique_ptr<std::remove_pointer_t<HDC>, DcDeleter>;
        using UniqueBitmap = std::unique_ptr<std::remove_pointer_t<HBITMAP>, BitmapDeleter>;

        class ScopedSelectObject {
        public:
            ScopedSelectObject() noexcept = default;
            ScopedSelectObject(HDC hdc, HGDIOBJ obj) noexcept
                : m_hdc(hdc), m_old(::SelectObject(hdc, obj)) {
            }
            ~ScopedSelectObject() noexcept { Restore(); }

            ScopedSelectObject(const ScopedSelectObject&) = delete;
            ScopedSelectObject& operator=(const ScopedSelectObject&) = delete;

            ScopedSelectObject(ScopedSelectObject&& o) noexcept
                : m_hdc(std::exchange(o.m_hdc, nullptr))
                , m_old(std::exchange(o.m_old, nullptr)) {
            }

            ScopedSelectObject& operator=(ScopedSelectObject&& o) noexcept {
                if (this != &o) {
                    Restore();
                    m_hdc = std::exchange(o.m_hdc, nullptr);
                    m_old = std::exchange(o.m_old, nullptr);
                }
                return *this;
            }

            bool IsValid() const noexcept { return m_hdc && m_old; }

        private:
            void Restore() noexcept {
                if (m_hdc && m_old) ::SelectObject(m_hdc, m_old);
            }

            HDC m_hdc = nullptr;
            HGDIOBJ m_old = nullptr;
        };

        struct AlphaDC {
            UniqueDc dc;
            UniqueBitmap bitmap;
            ScopedSelectObject selection;
            void* bits = nullptr;
            int width = 0;
            int height = 0;
            int stride = 0;

            bool IsValid() const noexcept { return dc && bitmap && bits; }
            HDC GetDC() const noexcept { return dc.get(); }
            HBITMAP GetBitmap() const noexcept { return bitmap.get(); }
            void Reset() noexcept { *this = {}; }
        };

        inline UniqueDc CreateMemoryDC() noexcept {
            return UniqueDc(::CreateCompatibleDC(nullptr));
        }

        inline UniqueBitmap CreateAlphaBitmap(
            HDC hdc, int w, int h, void** bits = nullptr) noexcept
        {
            BITMAPINFO bmi{};
            bmi.bmiHeader = { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB };
            return UniqueBitmap(::CreateDIBSection(
                hdc, &bmi, DIB_RGB_COLORS, bits, nullptr, 0));
        }

        inline AlphaDC CreateAlphaDC(int w, int h) noexcept {
            auto dc = CreateMemoryDC();
            void* bits = nullptr;
            auto bmp = CreateAlphaBitmap(dc.get(), w, h, &bits);
            ScopedSelectObject sel(dc.get(), bmp.get());
            return { std::move(dc), std::move(bmp), std::move(sel), bits, w, h, w * 4 };
        }
    }

    namespace Utils {
        template<typename E>
        inline E CycleEnum(E cur, int dir) {
            using U = std::underlying_type_t<E>;
            const auto n = U(E::Count);
            return E(((U(cur) + dir) % n + n) % n);
        }

        inline std::string_view ToString(FFTWindowType t) {
            constexpr std::string_view names[] = {
                "Hann", "Hamming", "Blackman", "Rectangular"
            };
            return names[size_t(t)];
        }

        inline std::string_view ToString(SpectrumScale t) {
            constexpr std::string_view names[] = { "Linear", "Logarithmic", "Mel" };
            return names[size_t(t)];
        }

        class Timer {
        public:
            Timer() : m_t(std::chrono::steady_clock::now()) {}
            void Reset() noexcept { m_t = std::chrono::steady_clock::now(); }

            float GetElapsedSeconds() const noexcept {
                return std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - m_t).count();
            }

            float GetElapsedMilliseconds() const noexcept {
                return GetElapsedSeconds() * 1000.0f;
            }

        private:
            std::chrono::steady_clock::time_point m_t;
        };

        class Random {
        public:
            static Random& Instance() {
                thread_local Random inst;
                return inst;
            }

            Random() : m_gen(std::random_device{}()), m_unit(0.0f, 1.0f) {}

            float Float(float lo = 0.0f, float hi = 1.0f) {
                return lo + m_unit(m_gen) * (hi - lo);
            }

            int Int(int lo, int hi) {
                return std::uniform_int_distribution<int>(lo, hi)(m_gen);
            }

            bool Bool(float p = 0.5f) { return m_unit(m_gen) < p; }

        private:
            std::mt19937 m_gen;
            std::uniform_real_distribution<float> m_unit;
        };
    }

    namespace Window {
        using Constants::kMinWindowSize;
        using Constants::kMaxWindowSize;

        inline bool IsWindowValid(HWND h) noexcept { return h && ::IsWindow(h); }

        inline bool IsValidSize(int w, int h) noexcept {
            return w >= kMinWindowSize && w <= kMaxWindowSize
                && h >= kMinWindowSize && h <= kMaxWindowSize;
        }

        struct WindowRect {
            int left, top, right, bottom;

            constexpr int Width() const noexcept { return right - left; }
            constexpr int Height() const noexcept { return bottom - top; }
            constexpr bool IsValid() const noexcept { return Width() > 0 && Height() > 0; }
            constexpr RECT ToRECT() const noexcept { return { left, top, right, bottom }; }

            static constexpr WindowRect FromRECT(const RECT& r) noexcept {
                return { r.left, r.top, r.right, r.bottom };
            }
        };

        inline std::optional<WindowRect> GetClientRect(HWND h) noexcept {
            RECT rc;
            return ::GetClientRect(h, &rc)
                ? std::optional(WindowRect::FromRECT(rc))
                : std::nullopt;
        }

        inline bool HideWindow(HWND h) noexcept { return ::ShowWindow(h, SW_HIDE); }

        inline bool ShowWindowState(HWND h, int cmd = SW_SHOW) noexcept {
            return ::ShowWindow(h, cmd);
        }

        inline bool CenterWindow(HWND h) noexcept {
            RECT rc;
            ::GetWindowRect(h, &rc);
            const int x = (::GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
            const int y = (::GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
            return ::SetWindowPos(h, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER) != 0;
        }

        inline bool PositionAtBottom(HWND h, int height) noexcept {
            return ::SetWindowPos(
                h, HWND_TOPMOST, 0,
                ::GetSystemMetrics(SM_CYSCREEN) - height,
                ::GetSystemMetrics(SM_CXSCREEN), height,
                SWP_SHOWWINDOW) != 0;
        }

        inline bool IsActiveAndVisible(HWND h) noexcept {
            return ::IsWindowVisible(h) && !::IsIconic(h);
        }
    }

} // namespace Spectrum::Helpers

namespace Spectrum {
    using Helpers::Math::Clamp;
    using Helpers::Math::Saturate;
    using Helpers::Math::Lerp;
    using Helpers::Math::Map;
    using Helpers::Math::Normalize;
    using Helpers::Math::SmoothStep;
    using Helpers::Math::DegreesToRadians;
    using Helpers::Math::RadiansToDegrees;
    using Helpers::Geometry::Distance;
    using Helpers::Geometry::DistanceSquared;
    using Helpers::Geometry::Length;
    using Helpers::Geometry::PointOnCircle;
    using Helpers::ColorHelpers::ColorToARGB;
    using Helpers::ColorHelpers::ARGBtoColor;
    using Helpers::ColorHelpers::InterpolateColor;
    using Helpers::ColorHelpers::AdjustBrightness;
    using Helpers::ColorHelpers::AdjustSaturation;
    using Helpers::ColorHelpers::AdjustAlpha;
    using Helpers::ColorHelpers::FloatToByte;
    using Helpers::ColorHelpers::ByteToFloat;
}

#endif