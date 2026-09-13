#ifndef SPECTRUM_CPP_RENDER_UTILS_H
#define SPECTRUM_CPP_RENDER_UTILS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Draw, spectrum / LED / polyline utilities
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Resource.h"

#include <blend2d.h>
#include <kfr/base.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace Spectrum::Helpers {

    namespace Geometry {
        constexpr Point Add(const Point& a, const Point& b) noexcept {
            return { a.x + b.x, a.y + b.y };
        }

        constexpr Point Multiply(const Point& p, float s) noexcept {
            return { p.x * s, p.y * s };
        }

        inline Point PointOnCircle(const Point& c, float r, float a) noexcept {
            return Add(c, { r * std::cos(a), r * std::sin(a) });
        }

        inline Point PointOnEllipse(const Point& c, float rx, float ry, float a) noexcept {
            return Add(c, { rx * std::cos(a), ry * std::sin(a) });
        }

        constexpr float GetRight(const Rect& r) noexcept { return r.x + r.width; }
        constexpr float GetBottom(const Rect& r) noexcept { return r.y + r.height; }

        constexpr Point GetCenter(const Rect& r) noexcept {
            return { r.x + r.width * 0.5f, r.y + r.height * 0.5f };
        }

        constexpr Point GetTopLeft(const Rect& r) noexcept { return { r.x, r.y }; }
        constexpr Point GetTopRight(const Rect& r) noexcept { return { GetRight(r), r.y }; }
        constexpr Point GetBottomRight(const Rect& r) noexcept { return { GetRight(r), GetBottom(r) }; }

        constexpr bool Contains(const Rect& r, const Point& p) noexcept {
            return p.x >= r.x && p.x <= GetRight(r) && p.y >= r.y && p.y <= GetBottom(r);
        }

        constexpr bool IsValid(const Rect& r) noexcept {
            return r.width > 0.0f && r.height > 0.0f;
        }

        constexpr Rect CreateCentered(const Point& c, float w, float h) noexcept {
            return { c.x - w * 0.5f, c.y - h * 0.5f, w, h };
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

        inline void OffsetPoints(std::vector<Point>& pts, float dx, float dy) noexcept {
            for (auto& p : pts) {
                p.x += dx;
                p.y += dy;
            }
        }
    }

    namespace ColorHelpers {
        constexpr uint8_t FloatToByte(float v) noexcept {
            return uint8_t(Clamp(v * 255.0f + 0.5f, 0.0f, 255.0f));
        }

        inline ::Spectrum::Color InterpolateColor(
            const ::Spectrum::Color& a, const ::Spectrum::Color& b, float t) noexcept
        {
            t = Saturate(t);
            return {
                Lerp(a.r, b.r, t),
                Lerp(a.g, b.g, t),
                Lerp(a.b, b.b, t),
                Lerp(a.a, b.a, t)
            };
        }

        inline ::Spectrum::Color AdjustBrightness(const ::Spectrum::Color& c, float f) noexcept {
            return {
                Saturate(c.r * f),
                Saturate(c.g * f),
                Saturate(c.b * f),
                c.a
            };
        }

        inline ::Spectrum::Color AdjustSaturation(const ::Spectrum::Color& c, float f) noexcept {
            const float gray = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
            return {
                Lerp(gray, c.r, f),
                Lerp(gray, c.g, f),
                Lerp(gray, c.b, f),
                c.a
            };
        }

        inline ::Spectrum::Color AdjustAlpha(const ::Spectrum::Color& c, float a) noexcept {
            return { c.r, c.g, c.b, Saturate(a) };
        }

        inline ::Spectrum::Color SampleGradient(
            const std::vector<::Spectrum::Color>& g, float t) noexcept
        {
            if (g.empty())
                return {};
            if (g.size() == 1)
                return g[0];
            const float s = Saturate(t) * float(g.size() - 1);
            const size_t i = size_t(s);
            return InterpolateColor(g[i], g[std::min(i + 1, g.size() - 1)], s - float(i));
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

        private:
            void Restore() noexcept {
                if (m_hdc && m_old)
                    ::SelectObject(m_hdc, m_old);
            }

            HDC     m_hdc = nullptr;
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

        inline UniqueBitmap CreateAlphaBitmap(HDC hdc, int w, int h, void** bits = nullptr) noexcept {
            BITMAPINFO bmi{};
            bmi.bmiHeader = { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB };
            return UniqueBitmap(::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, bits, nullptr, 0));
        }

        inline AlphaDC CreateAlphaDC(int w, int h) noexcept {
            auto dc = CreateMemoryDC();
            void* bits = nullptr;
            auto bmp = CreateAlphaBitmap(dc.get(), w, h, &bits);
            ScopedSelectObject sel(dc.get(), bmp.get());
            return { std::move(dc), std::move(bmp), std::move(sel), bits, w, h, w * 4 };
        }
    }

    namespace Window {
        struct WindowRect {
            int left, top, right, bottom;

            constexpr int Width() const noexcept { return right - left; }
            constexpr int Height() const noexcept { return bottom - top; }
            constexpr RECT ToRECT() const noexcept { return { left, top, right, bottom }; }

            static constexpr WindowRect FromRECT(const RECT& r) noexcept {
                return { r.left, r.top, r.right, r.bottom };
            }
        };

        inline std::optional<WindowRect> GetClientRect(HWND h) noexcept {
            RECT rc;
            if (!::GetClientRect(h, &rc))
                return std::nullopt;
            return WindowRect::FromRECT(rc);
        }

        inline bool HideWindow(HWND h) noexcept { return ::ShowWindow(h, SW_HIDE); }
        inline bool ShowWindowState(HWND h, int cmd = SW_SHOW) noexcept { return ::ShowWindow(h, cmd); }

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
    using Helpers::Geometry::PointOnCircle;
    using Helpers::Geometry::PointOnEllipse;
    using Helpers::Geometry::Add;
    using Helpers::Geometry::Multiply;
    using Helpers::Geometry::GetTopLeft;
    using Helpers::Geometry::GetTopRight;
    using Helpers::Geometry::GetBottomRight;
    using Helpers::Geometry::GetBottom;
    using Helpers::Geometry::GetCenter;
    using Helpers::Geometry::Deflate;
    using Helpers::Geometry::CreateCentered;
    using Helpers::Geometry::OffsetPoints;
    using Helpers::ColorHelpers::InterpolateColor;
    using Helpers::ColorHelpers::AdjustBrightness;
    using Helpers::ColorHelpers::AdjustSaturation;
    using Helpers::ColorHelpers::AdjustAlpha;
    using Helpers::ColorHelpers::FloatToByte;
    using Helpers::ColorHelpers::SampleGradient;
}

namespace Spectrum::Draw {
    namespace detail {

        struct ScopedStroke {
            BLContext& ctx;

            ScopedStroke(BLContext& c, float w, bool caps, bool join) : ctx(c) {
                ctx.save();
                ctx.set_stroke_width(w);
                if (caps)
                    ctx.set_stroke_caps(BL_STROKE_CAP_ROUND);
                if (join)
                    ctx.set_stroke_join(BL_STROKE_JOIN_ROUND);
            }

            ~ScopedStroke() { ctx.restore(); }

            ScopedStroke(const ScopedStroke&) = delete;
            ScopedStroke& operator=(const ScopedStroke&) = delete;
        };

        inline BLPath Polyline(const std::vector<Point>& pts, bool closed) {
            BLPath path;
            if (pts.empty())
                return path;
            path.move_to(pts[0].x, pts[0].y);
            for (size_t i = 1; i < pts.size(); ++i)
                path.line_to(pts[i].x, pts[i].y);
            if (closed)
                path.close();
            return path;
        }

    } // namespace detail

    inline BLRgba32 ToBL(const Color& c) noexcept {
        return BLRgba32(FloatToByte(c.r), FloatToByte(c.g), FloatToByte(c.b), FloatToByte(c.a));
    }

    inline BLPoint ToBL(const Point& p) noexcept { return { p.x, p.y }; }
    inline BLRect  ToBL(const Rect& r)  noexcept { return { r.x, r.y, r.width, r.height }; }

    inline void FillRect(BLContext& ctx, const Rect& r, const Color& c) {
        ctx.fill_rect(ToBL(r), ToBL(c));
    }

    inline void FillRoundRect(BLContext& ctx, const Rect& r, float radius, const Color& c) {
        ctx.fill_round_rect({ r.x, r.y, r.width, r.height, radius, radius }, ToBL(c));
    }

    inline void FillCircle(BLContext& ctx, const Point& c, float r, const Color& color) {
        ctx.fill_circle({ c.x, c.y, r }, ToBL(color));
    }

    inline void FillPolygon(BLContext& ctx, const std::vector<Point>& pts, const Color& c) {
        if (pts.size() < 3)
            return;
        ctx.fill_path(detail::Polyline(pts, true), ToBL(c));
    }

    inline void StrokeCircle(BLContext& ctx, const Point& c, float r, const Color& color, float w = 1.0f) {
        detail::ScopedStroke s(ctx, w, false, false);
        ctx.stroke_circle({ c.x, c.y, r }, ToBL(color));
    }

    inline void StrokeLine(BLContext& ctx, const Point& a, const Point& b, const Color& c, float w = 1.0f) {
        detail::ScopedStroke s(ctx, w, true, false);
        ctx.stroke_line({ a.x, a.y, b.x, b.y }, ToBL(c));
    }

    inline void StrokePolyline(BLContext& ctx, const std::vector<Point>& pts, const Color& c, float w = 1.0f) {
        if (pts.size() < 2)
            return;
        detail::ScopedStroke s(ctx, w, true, true);
        ctx.stroke_path(detail::Polyline(pts, false), ToBL(c));
    }

    inline void Glow(
        BLContext& ctx, const Point& c, float r, const Color& color,
        float intensity = 1.0f, int layers = 5)
    {
        for (int i = layers; i > 0; --i) {
            const float a = (1.f - float(i) / float(layers)) * 0.2f * intensity * color.a;
            FillCircle(ctx, c, r + i * 2.f, Color(color.r, color.g, color.b, a));
        }
    }

    inline const BLFont* UiFont(float size) {
        static BLFontFace face;
        static BLFont font;
        static float last = -1.f;

        if (last < 0.f) {
            const HRSRC r = FindResourceW(
                nullptr, MAKEINTRESOURCEW(IDR_FONT_PIXEL_OPERATOR), RT_RCDATA);
            BLFontData data;
            data.create_from_data(
                LockResource(LoadResource(nullptr, r)), SizeofResource(nullptr, r));
            face.create_from_data(data, 0);
        }

        if (last != size) {
            font.create_from_face(face, size);
            last = size;
        }
        return &font;
    }

    inline void FillTextCentered(
        BLContext& ctx, std::string_view utf8,
        const Rect& rect, const Color& color, float size)
    {
        const BLFont& font = *UiFont(size);
        BLGlyphBuffer gb;
        gb.set_utf8_text(utf8.data(), utf8.size());
        font.shape(gb);

        BLTextMetrics tm{};
        font.get_text_metrics(gb, tm);
        const auto fm = font.metrics();

        const float x = rect.x
            + (rect.width - float(tm.bounding_box.x1 - tm.bounding_box.x0)) * 0.5f
            - float(tm.bounding_box.x0);
        const float y = rect.y + (rect.height + fm.ascent - fm.descent) * 0.5f;

        ctx.fill_utf8_text(BLPoint(x, y), font, utf8.data(), utf8.size(), ToBL(color));
    }

} // namespace Spectrum::Draw

namespace Spectrum::RenderUtils {

    using Spectrum::ResampleSpectrum;

    constexpr float kShadowOffset = 2.0f;
    constexpr float kLedBlend = 0.7f;
    constexpr float kLedMinMag = 0.05f;

    enum class RoundingMode { None, All, Top, Bottom };

    using ColorGradient = std::vector<Color>;
    using RectBatch = std::map<Color, std::vector<Rect>>;
    using PointBatch = std::map<Color, std::vector<Point>>;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Shadow
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline Color ShadowColor() noexcept {
        return { 0.0f, 0.0f, 0.0f, 0.35f };
    }

    [[nodiscard]] inline Rect OffsetRect(
        const Rect& r, float ox = kShadowOffset, float oy = kShadowOffset) noexcept
    {
        return { r.x + ox, r.y + oy, r.width, r.height };
    }

    inline void OffsetShadow(std::vector<Point>& pts) noexcept {
        OffsetPoints(pts, kShadowOffset, kShadowOffset);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline int GetMaxBarsForQuality(RenderQuality quality) noexcept {
        switch (quality) {
        case RenderQuality::Low:   return 16;
        case RenderQuality::High:  return 48;
        case RenderQuality::Ultra: return 64;
        default:                   return 32;
        }
    }

    [[nodiscard]] inline float MagnitudeToHeight(float mag, float h, float scale) noexcept {
        return Normalized(mag) * h * scale;
    }

    [[nodiscard]] inline float GetAverageMagnitude(const SpectrumData& spectrum) noexcept {
        if (spectrum.empty())
            return 0.0f;
        return static_cast<float>(kfr::mean(kfr::make_univector(spectrum)));
    }

    [[nodiscard]] inline float SegmentAverage(
        const SpectrumData& spectrum, size_t segments, size_t index) noexcept
    {
        if (spectrum.empty())
            return 0.0f;
        if (segments == 0)
            return 0.0f;
        if (index >= segments)
            return 0.0f;
        const size_t start = index * spectrum.size() / segments;
        const size_t end = (index + 1) * spectrum.size() / segments;
        if (start >= end)
            return 0.0f;
        return static_cast<float>(
            kfr::mean(kfr::make_univector(spectrum.data() + start, end - start)));
    }

    [[nodiscard]] inline float RmsToDb(
        const SpectrumData& spectrum, float minDb, float maxDb) noexcept
    {
        if (spectrum.empty())
            return minDb;
        const float rms = static_cast<float>(kfr::rms(kfr::make_univector(spectrum)));
        return Clamp(20.0f * std::log10(std::max(rms, 1e-10f)), minDb, maxDb);
    }

    inline void BuildPolylineFromSpectrum(
        const SpectrumData& src,
        float midY,
        float amplitude,
        int width,
        std::vector<Point>& out)
    {
        if (width <= 0 || src.empty()) {
            out.clear();
            return;
        }
        out.resize(size_t(width));
        const float last = float(src.size() - 1);
        float denom = 1.0f;
        if (width > 1)
            denom = float(width - 1);
        for (int x = 0; x < width; ++x) {
            const float t = float(x) / denom * last;
            const size_t i0 = size_t(t);
            size_t i1 = i0;
            if (i0 + 1 < src.size())
                i1 = i0 + 1;
            const float v = src[i0] + (src[i1] - src[i0]) * (t - float(i0));
            out[size_t(x)] = { float(x), midY - v * amplitude };
        }
    }

    [[nodiscard]] inline Color IntensityColor(const Color& base, float intensity) noexcept {
        if (intensity <= 0.7f)
            return base;
        return AdjustBrightness(
            base, Lerp(1.0f, 1.3f, Map(intensity, 0.7f, 1.0f, 0.0f, 1.0f)));
    }

    [[nodiscard]] inline std::vector<Point> CircularPoints(const Point& c, float r, size_t n) {
        std::vector<Point> pts;
        pts.reserve(n);
        const float step = TWO_PI / float(n);
        for (size_t i = 0; i < n; ++i)
            pts.push_back(PointOnCircle(c, r, i * step));
        return pts;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Draw helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline void DrawRoundedRect(
        BLContext& ctx,
        const Rect& r,
        float radius,
        const Color& color,
        RoundingMode mode = RoundingMode::All)
    {
        if (radius <= 0.0f || mode == RoundingMode::None) {
            Draw::FillRect(ctx, r, color);
            return;
        }
        if (mode == RoundingMode::All) {
            Draw::FillRoundRect(ctx, r, radius, color);
            return;
        }
        if (r.height < radius * 2.0f) {
            Draw::FillRect(ctx, r, color);
            return;
        }

        const bool top = (mode == RoundingMode::Top);
        float bodyY = r.y;
        float capY = r.y + r.height - radius * 2.0f;
        if (top) {
            bodyY = r.y + radius;
            capY = r.y;
        }
        Draw::FillRect(ctx, { r.x, bodyY, r.width, r.height - radius }, color);
        Draw::FillRoundRect(ctx, { r.x, capY, r.width, radius * 2.0f }, radius, color);
    }

    inline void RenderWithShadow(
        BLContext& ctx,
        const std::function<void()>& draw,
        const Point& offset = { 2.0f, 2.0f },
        float alpha = 0.3f)
    {
        ctx.save();
        ctx.translate(offset.x, offset.y);
        ctx.set_global_alpha(double(alpha));
        draw();
        ctx.restore();
        draw();
    }

    inline void RenderRectBatches(
        BLContext& ctx,
        const RectBatch& batches,
        float cornerRadius = 0.0f,
        RoundingMode mode = RoundingMode::All)
    {
        for (const auto& [color, rects] : batches)
            for (const auto& r : rects)
                DrawRoundedRect(ctx, r, cornerRadius, color, mode);
    }

    inline void RenderCircleBatches(BLContext& ctx, const PointBatch& batches, float radius) {
        for (const auto& [color, pts] : batches)
            for (const auto& p : pts)
                Draw::FillCircle(ctx, p, radius, color);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // LED
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline std::vector<Color> LedGradient() {
        return {
            Color::FromRGB(20, 180, 70),
            Color::FromRGB(180, 210, 40),
            Color::FromRGB(255, 170, 0),
            Color::FromRGB(230, 40, 40)
        };
    }

    [[nodiscard]] inline Color LedIdleColor(bool overlay = false) noexcept {
        float a = 0.32f;
        if (overlay)
            a = 0.16f;
        return { 0.10f, 0.10f, 0.14f, a };
    }

    [[nodiscard]] inline int LitRows(float mag, int rows) noexcept {
        if (rows <= 0)
            return 0;
        return Clamp(int(Normalized(mag) * float(rows) + 0.5f), 0, rows);
    }

    [[nodiscard]] inline float RowT(int row, int rows) noexcept {
        if (rows > 1)
            return float(row) / float(rows - 1);
        return 1.0f;
    }

    [[nodiscard]] inline float LedBrightness(float mag, bool isTop) noexcept {
        if (isTop)
            return Clamp(0.45f + mag * 0.55f, 0.0f, 1.0f);
        return 1.0f;
    }

    [[nodiscard]] inline int PeakRow(float peak, int rows) noexcept {
        return LitRows(peak, rows) - 1;
    }

    [[nodiscard]] inline bool IsValidRow(int row, int rows) noexcept {
        return row >= 0 && row < rows;
    }

    [[nodiscard]] inline Color BlendLedPrimary(
        const Color& gradient, const Color& primary, float rowT, float blend = kLedBlend) noexcept
    {
        if (primary.r == 1.0f && primary.g == 1.0f && primary.b == 1.0f)
            return gradient;
        return InterpolateColor(primary, gradient, rowT * (1.0f - blend) + blend);
    }

    [[nodiscard]] inline Color LedColor(
        const std::vector<Color>& gradient, float rowT, float brightness) noexcept
    {
        return AdjustAlpha(SampleGradient(gradient, rowT), brightness);
    }

    template<typename Batch, typename CellFn>
    void FillIdleGrid(Batch& batch, int cols, int rows, const Color& idle, CellFn&& cell) {
        for (int col = 0; col < cols; ++col)
            for (int row = 0; row < rows; ++row)
                batch[idle].push_back(cell(col, row));
    }

    template<typename Fn>
    void ForEachLitLed(float mag, int rows, bool forceMin, Fn&& fn) {
        int lit = LitRows(mag, rows);
        if (lit == 0 && forceMin && mag > kLedMinMag)
            lit = 1;
        if (lit == 0)
            return;
        for (int row = 0; row < lit; ++row)
            fn(row, lit);
    }

} // namespace Spectrum::RenderUtils

namespace Spectrum {
    using RenderUtils::RoundingMode;
    using RenderUtils::ColorGradient;
    using RenderUtils::RectBatch;
    using RenderUtils::PointBatch;
    using RenderUtils::CircularPoints;
    using RenderUtils::DrawRoundedRect;
    using RenderUtils::RenderWithShadow;
    using RenderUtils::RenderRectBatches;
    using RenderUtils::RenderCircleBatches;
}

#endif