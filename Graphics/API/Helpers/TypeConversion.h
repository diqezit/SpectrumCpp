#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_TYPE_CONVERSION_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_TYPE_CONVERSION_H

#include "Common/Types.h"
#include <d2d1.h>

namespace Spectrum::Helpers::TypeConversion {

    [[nodiscard]] inline D2D1_COLOR_F ToD2DColor(const Color& c) noexcept
    {
        // Premultiply alpha for Direct2D
        return D2D1::ColorF(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
    }

    [[nodiscard]] inline D2D1_POINT_2F ToD2DPoint(const Point& p) noexcept
    {
        return D2D1::Point2F(p.x, p.y);
    }

    [[nodiscard]] inline D2D1_RECT_F ToD2DRect(const Rect& r) noexcept
    {
        return D2D1::RectF(r.x, r.y, r.GetRight(), r.GetBottom());
    }

    [[nodiscard]] inline D2D1_SIZE_F ToD2DSize(float width, float height) noexcept
    {
        return D2D1::SizeF(width, height);
    }

    [[nodiscard]] inline D2D1_SIZE_U ToD2DSizeU(UINT32 width, UINT32 height) noexcept
    {
        return D2D1::SizeU(width, height);
    }

    [[nodiscard]] inline D2D1_ELLIPSE ToD2DEllipse(const Point& center, float radiusX, float radiusY) noexcept
    {
        return D2D1::Ellipse(ToD2DPoint(center), radiusX, radiusY);
    }

} // namespace Spectrum::Helpers::TypeConversion

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_TYPE_CONVERSION_H