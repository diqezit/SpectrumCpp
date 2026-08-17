#ifndef SPECTRUM_CPP_DRAW_H
#define SPECTRUM_CPP_DRAW_H

#include "Graphics/API/GraphicsHelpers.h"

#include <blend2d.h>
#include <string_view>
#include <vector>

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
            path.move_to(pts[0].x, pts[0].y);
            for (size_t i = 1; i < pts.size(); ++i)
                path.line_to(pts[i].x, pts[i].y);
            if (closed)
                path.close();
            return path;
        }

        inline BLPath Arc(const Point& c, float r, float a, float s, bool pie) {
            BLPath path;
            if (pie)
                path.move_to(c.x, c.y);
            path.arc_to(c.x, c.y, r, r, DegreesToRadians(a), DegreesToRadians(s), false);
            if (pie)
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

    inline void FillEllipse(BLContext& ctx, const Point& c, float rx, float ry, const Color& color) {
        ctx.fill_ellipse({ c.x, c.y, rx, ry }, ToBL(color));
    }

    inline void FillPolygon(BLContext& ctx, const std::vector<Point>& pts, const Color& c) {
        ctx.fill_path(detail::Polyline(pts, true), ToBL(c));
    }

    inline void FillSector(
        BLContext& ctx, const Point& c, float r,
        float startDeg, float sweepDeg, const Color& color)
    {
        ctx.fill_path(detail::Arc(c, r, startDeg, sweepDeg, true), ToBL(color));
    }

    inline void StrokeRect(BLContext& ctx, const Rect& r, const Color& c, float w = 1.0f) {
        detail::ScopedStroke s(ctx, w, false, false);
        ctx.stroke_rect(ToBL(r), ToBL(c));
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
        detail::ScopedStroke s(ctx, w, true, true);
        ctx.stroke_path(detail::Polyline(pts, false), ToBL(c));
    }

    inline void StrokePolygon(BLContext& ctx, const std::vector<Point>& pts, const Color& c, float w = 1.0f) {
        detail::ScopedStroke s(ctx, w, false, true);
        ctx.stroke_path(detail::Polyline(pts, true), ToBL(c));
    }

    inline void StrokeArc(
        BLContext& ctx, const Point& c, float r,
        float startDeg, float sweepDeg, const Color& color, float w = 1.0f)
    {
        detail::ScopedStroke s(ctx, w, true, false);
        ctx.stroke_path(detail::Arc(c, r, startDeg, sweepDeg, false), ToBL(color));
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
            for (auto* p : {
                "C:\\Windows\\Fonts\\seguisym.ttf",
                "C:\\Windows\\Fonts\\segoeui.ttf",
                "C:\\Windows\\Fonts\\arial.ttf"
                }) {
                if (face.create_from_file(p) == BL_SUCCESS)
                    break;
            }
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

#endif