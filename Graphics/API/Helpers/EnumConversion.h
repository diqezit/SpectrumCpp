#ifndef SPECTRUM_CPP_ENUM_CONVERSION_H
#define SPECTRUM_CPP_ENUM_CONVERSION_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Provides conversion functions between Spectrum enums and Direct2D/DirectWrite
// native enumerations. This allows the rest of the codebase to use semantic
// enums while maintaining zero-cost abstraction to native APIs.
//
// Design notes:
// - All functions are inline for zero overhead
// - All functions are noexcept for performance
// - Uses switch statements for compile-time optimization
// - Provides reverse conversions where needed
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <d2d1.h>
#include <dwrite.h>
#include "Graphics/API/Enums/PaintEnums.h"
#include "Graphics/API/Enums/RenderEnums.h"
#include "Graphics/API/Enums/TextEnums.h"

namespace Spectrum::Helpers::EnumConversion {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paint Enums → Direct2D
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline D2D1_CAP_STYLE ToD2DCapStyle(StrokeCap cap) noexcept {
        switch (cap) {
        case StrokeCap::Flat:   return D2D1_CAP_STYLE_FLAT;
        case StrokeCap::Round:  return D2D1_CAP_STYLE_ROUND;
        case StrokeCap::Square: return D2D1_CAP_STYLE_SQUARE;
        }
        return D2D1_CAP_STYLE_FLAT;
    }

    [[nodiscard]] inline D2D1_LINE_JOIN ToD2DLineJoin(StrokeJoin join) noexcept {
        switch (join) {
        case StrokeJoin::Miter: return D2D1_LINE_JOIN_MITER;
        case StrokeJoin::Round: return D2D1_LINE_JOIN_ROUND;
        case StrokeJoin::Bevel: return D2D1_LINE_JOIN_BEVEL;
        }
        return D2D1_LINE_JOIN_MITER;
    }

    [[nodiscard]] inline D2D1_DASH_STYLE ToD2DDashStyle(DashStyle style) noexcept {
        switch (style) {
        case DashStyle::Solid:      return D2D1_DASH_STYLE_SOLID;
        case DashStyle::Dash:       return D2D1_DASH_STYLE_DASH;
        case DashStyle::Dot:        return D2D1_DASH_STYLE_DOT;
        case DashStyle::DashDot:    return D2D1_DASH_STYLE_DASH_DOT;
        case DashStyle::DashDotDot: return D2D1_DASH_STYLE_DASH_DOT_DOT;
        case DashStyle::Custom:     return D2D1_DASH_STYLE_CUSTOM;
        }
        return D2D1_DASH_STYLE_SOLID;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Render Enums → Direct2D
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline D2D1_ANTIALIAS_MODE ToD2DAntiAliasMode(AntiAliasMode mode) noexcept {
        switch (mode) {
        case AntiAliasMode::None:         return D2D1_ANTIALIAS_MODE_ALIASED;
        case AntiAliasMode::PerPrimitive: return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
        }
        return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
    }

    [[nodiscard]] inline D2D1_ANTIALIAS_MODE ToD2DAntiAliasMode(bool antiAlias) noexcept {
        return antiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED;
    }

    [[nodiscard]] inline D2D1_FILL_MODE ToD2DFillMode(FillRule rule) noexcept {
        switch (rule) {
        case FillRule::EvenOdd: return D2D1_FILL_MODE_ALTERNATE;
        case FillRule::Winding: return D2D1_FILL_MODE_WINDING;
        }
        return D2D1_FILL_MODE_WINDING;
    }

    [[nodiscard]] inline D2D1_BITMAP_INTERPOLATION_MODE ToD2DInterpolationMode(FilterQuality quality) noexcept {
        switch (quality) {
        case FilterQuality::None:
            return D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        case FilterQuality::Low:
        case FilterQuality::Medium:
        case FilterQuality::High:
            return D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
        }
        return D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Enums → DirectWrite
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline DWRITE_TEXT_ALIGNMENT ToDWriteTextAlign(TextAlign align) noexcept {
        switch (align) {
        case TextAlign::Leading:   return DWRITE_TEXT_ALIGNMENT_LEADING;
        case TextAlign::Trailing:  return DWRITE_TEXT_ALIGNMENT_TRAILING;
        case TextAlign::Center:    return DWRITE_TEXT_ALIGNMENT_CENTER;
        case TextAlign::Justified: return DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
        }
        return DWRITE_TEXT_ALIGNMENT_LEADING;
    }

    [[nodiscard]] inline DWRITE_PARAGRAPH_ALIGNMENT ToDWriteParagraphAlign(ParagraphAlign align) noexcept {
        switch (align) {
        case ParagraphAlign::Near:   return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        case ParagraphAlign::Far:    return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
        case ParagraphAlign::Center: return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        }
        return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    }

    [[nodiscard]] inline DWRITE_FONT_WEIGHT ToDWriteFontWeight(FontWeight weight) noexcept {
        return static_cast<DWRITE_FONT_WEIGHT>(weight);
    }

    [[nodiscard]] inline DWRITE_FONT_STYLE ToDWriteFontStyle(FontStyle style) noexcept {
        switch (style) {
        case FontStyle::Normal:  return DWRITE_FONT_STYLE_NORMAL;
        case FontStyle::Italic:  return DWRITE_FONT_STYLE_ITALIC;
        case FontStyle::Oblique: return DWRITE_FONT_STYLE_OBLIQUE;
        }
        return DWRITE_FONT_STYLE_NORMAL;
    }

    [[nodiscard]] inline DWRITE_FONT_STRETCH ToDWriteFontStretch(FontStretch stretch) noexcept {
        return static_cast<DWRITE_FONT_STRETCH>(stretch);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Reverse Conversions (Direct2D → Spectrum)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] inline StrokeCap FromD2DCapStyle(D2D1_CAP_STYLE cap) noexcept {
        switch (cap) {
        case D2D1_CAP_STYLE_FLAT:   return StrokeCap::Flat;
        case D2D1_CAP_STYLE_ROUND:  return StrokeCap::Round;
        case D2D1_CAP_STYLE_SQUARE: return StrokeCap::Square;
        case D2D1_CAP_STYLE_TRIANGLE: return StrokeCap::Flat; // No triangle in our enum
        }
        return StrokeCap::Flat;
    }

    [[nodiscard]] inline StrokeJoin FromD2DLineJoin(D2D1_LINE_JOIN join) noexcept {
        switch (join) {
        case D2D1_LINE_JOIN_MITER:          return StrokeJoin::Miter;
        case D2D1_LINE_JOIN_ROUND:          return StrokeJoin::Round;
        case D2D1_LINE_JOIN_BEVEL:          return StrokeJoin::Bevel;
        case D2D1_LINE_JOIN_MITER_OR_BEVEL: return StrokeJoin::Miter;
        }
        return StrokeJoin::Miter;
    }

} // namespace Spectrum::Helpers::EnumConversion

#endif // SPECTRUM_CPP_ENUM_CONVERSION_H