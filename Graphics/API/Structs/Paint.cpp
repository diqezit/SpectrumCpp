#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Brushes/SolidColorBrush.h"
#include "Graphics/API/Brushes/LinearGradientBrush.h"
#include "Graphics/API/Brushes/RadialGradientBrush.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Factory Methods for Brush Types
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Paint Paint::Fill(const Color& color) {
        Paint p;
        p.m_brush = std::make_shared<SolidColorBrush>(color);
        p.m_style = PaintStyle::Fill;
        return p;
    }

    Paint Paint::Stroke(const Color& color, float width) {
        Paint p;
        p.m_brush = std::make_shared<SolidColorBrush>(color);
        p.m_style = PaintStyle::Stroke;
        p.m_strokeWidth = width;
        return p;
    }

    Paint Paint::LinearGradient(
        const Point& start,
        const Point& end,
        const std::vector<GradientStop>& stops
    ) {
        Paint p;
        p.m_brush = std::make_shared<LinearGradientBrush>(start, end, stops);
        return p;
    }

    Paint Paint::RadialGradient(
        const Point& center,
        float radius,
        const std::vector<GradientStop>& stops
    ) {
        Paint p;
        p.m_brush = std::make_shared<RadialGradientBrush>(center, radius, stops);
        return p;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Immutable Builder Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Paint Paint::WithStyle(PaintStyle style) const {
        Paint result = *this;
        result.m_style = style;
        return result;
    }

    Paint Paint::WithStrokeWidth(float width) const {
        Paint result = *this;
        result.m_strokeWidth = std::max(0.0f, width);
        return result;
    }

    Paint Paint::WithStrokeCap(StrokeCap cap) const {
        Paint result = *this;
        result.m_strokeCap = cap;
        return result;
    }

    Paint Paint::WithStrokeJoin(StrokeJoin join) const {
        Paint result = *this;
        result.m_strokeJoin = join;
        return result;
    }

    Paint Paint::WithMiterLimit(float limit) const {
        Paint result = *this;
        result.m_miterLimit = std::max(0.0f, limit);
        return result;
    }

    Paint Paint::WithDashStyle(DashStyle style) const {
        Paint result = *this;
        result.m_dashStyle = style;
        return result;
    }

    Paint Paint::WithDashPattern(std::vector<float> pattern) const {
        Paint result = *this;
        result.m_dashPattern = std::move(pattern);
        result.m_dashStyle = DashStyle::Custom;
        return result;
    }

    Paint Paint::WithDashOffset(float offset) const {
        Paint result = *this;
        result.m_dashOffset = offset;
        return result;
    }

    Paint Paint::WithAntiAlias(bool enabled) const {
        Paint result = *this;
        result.m_antiAlias = enabled;
        return result;
    }

    Paint Paint::WithAlpha(float alpha) const {
        Paint result = *this;
        result.m_globalAlpha = std::clamp(alpha, 0.0f, 1.0f);
        return result;
    }

    Paint Paint::WithBlendMode(BlendMode mode) const {
        Paint result = *this;
        result.m_blendMode = mode;
        return result;
    }

    Paint Paint::WithFilterQuality(FilterQuality quality) const {
        Paint result = *this;
        result.m_filterQuality = quality;
        return result;
    }
}