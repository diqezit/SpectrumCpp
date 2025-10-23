#ifndef SPECTRUM_CPP_PAINT_STRUCT_H
#define SPECTRUM_CPP_PAINT_STRUCT_H

#include "Common/Types.h"
#include "Graphics/API/Enums/PaintEnums.h"
#include "Graphics/API/Enums/RenderEnums.h"
#include "Graphics/API/Brushes/IBrush.h"
#include <memory>
#include <vector>
#include <optional>
#include <algorithm>

namespace Spectrum {

    // Forward declarations
    struct GradientStop;

    class Paint {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Constructors and Factories
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Paint() = default;

        [[nodiscard]] static Paint Fill(const Color& color);
        [[nodiscard]] static Paint Stroke(const Color& color, float width = 1.0f);

        [[nodiscard]] static Paint LinearGradient(
            const Point& start,
            const Point& end,
            const std::vector<GradientStop>& stops
        );

        [[nodiscard]] static Paint RadialGradient(
            const Point& center,
            float radius,
            const std::vector<GradientStop>& stops
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Immutable Builder Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Paint WithStyle(PaintStyle style) const;
        [[nodiscard]] Paint WithStrokeWidth(float width) const;
        [[nodiscard]] Paint WithStrokeCap(StrokeCap cap) const;
        [[nodiscard]] Paint WithStrokeJoin(StrokeJoin join) const;
        [[nodiscard]] Paint WithMiterLimit(float limit) const;
        [[nodiscard]] Paint WithDashStyle(DashStyle style) const;
        [[nodiscard]] Paint WithDashPattern(std::vector<float> pattern) const;
        [[nodiscard]] Paint WithDashOffset(float offset) const;
        [[nodiscard]] Paint WithAntiAlias(bool enabled) const;
        [[nodiscard]] Paint WithAlpha(float alpha) const;
        [[nodiscard]] Paint WithBlendMode(BlendMode mode) const;
        [[nodiscard]] Paint WithFilterQuality(FilterQuality quality) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] const std::shared_ptr<IBrush>& GetBrush() const noexcept { return m_brush; }
        [[nodiscard]] PaintStyle GetStyle() const noexcept { return m_style; }
        [[nodiscard]] float GetStrokeWidth() const noexcept { return m_strokeWidth; }
        [[nodiscard]] StrokeCap GetStrokeCap() const noexcept { return m_strokeCap; }
        [[nodiscard]] StrokeJoin GetStrokeJoin() const noexcept { return m_strokeJoin; }
        [[nodiscard]] float GetMiterLimit() const noexcept { return m_miterLimit; }
        [[nodiscard]] DashStyle GetDashStyle() const noexcept { return m_dashStyle; }
        [[nodiscard]] const std::vector<float>& GetDashPattern() const noexcept { return m_dashPattern; }
        [[nodiscard]] float GetDashOffset() const noexcept { return m_dashOffset; }
        [[nodiscard]] bool IsAntiAlias() const noexcept { return m_antiAlias; }
        [[nodiscard]] float GetAlpha() const noexcept { return m_globalAlpha; }
        [[nodiscard]] BlendMode GetBlendMode() const noexcept { return m_blendMode; }
        [[nodiscard]] FilterQuality GetFilterQuality() const noexcept { return m_filterQuality; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Convenience Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsFilled() const noexcept {
            return m_style == PaintStyle::Fill || m_style == PaintStyle::FillAndStroke;
        }

        [[nodiscard]] bool IsStroked() const noexcept {
            return m_style == PaintStyle::Stroke || m_style == PaintStyle::FillAndStroke;
        }

        [[nodiscard]] bool NeedsStrokeStyle() const noexcept {
            return m_strokeCap != StrokeCap::Flat ||
                m_strokeJoin != StrokeJoin::Miter ||
                m_dashStyle != DashStyle::Solid ||
                m_miterLimit != 10.0f;
        }

    private:
        // Visual properties
        std::shared_ptr<IBrush> m_brush;
        PaintStyle m_style = PaintStyle::Fill;

        // Stroke properties
        float m_strokeWidth = 1.0f;
        StrokeCap m_strokeCap = StrokeCap::Flat;
        StrokeJoin m_strokeJoin = StrokeJoin::Miter;
        float m_miterLimit = 10.0f;

        // Dash properties
        DashStyle m_dashStyle = DashStyle::Solid;
        std::vector<float> m_dashPattern;
        float m_dashOffset = 0.0f;

        // Rendering properties
        bool m_antiAlias = true;
        float m_globalAlpha = 1.0f;
        BlendMode m_blendMode = BlendMode::SourceOver;
        FilterQuality m_filterQuality = FilterQuality::Low;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PAINT_STRUCT_H