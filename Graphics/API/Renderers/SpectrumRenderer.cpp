// SpectrumRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the SpectrumRenderer class. This file contains high-level
// logic for drawing spectrum data, delegating primitive and gradient
// operations to specialized renderer components.
//
// Implementation details:
// - Bar rendering validates dimensions and skips invisible bars
// - Waveform generation delegated to GeometryBuilder
// - Mirror effect achieved via vertical reflection with transparency
// - Uses D2DHelpers for validation and sanitization
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "SpectrumRenderer.h"
#include "D2DHelpers.h"

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    SpectrumRenderer::SpectrumRenderer(
        PrimitiveRenderer* primitiveRenderer,
        GradientRenderer* gradientRenderer,
        GeometryBuilder* geometryBuilder
    )
        : m_primitiveRenderer(primitiveRenderer)
        , m_gradientRenderer(gradientRenderer)
        , m_geometryBuilder(geometryBuilder)
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Visualization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SpectrumRenderer::DrawSpectrumBars(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const BarStyle& style,
        const Color& color
    ) const
    {
        if (spectrum.empty()) return;

        const size_t barCount = spectrum.size();
        if (barCount == 0) return;

        const float totalBarWidth = bounds.width / static_cast<float>(barCount);
        const float barWidth = totalBarWidth - style.spacing;

        if (barWidth <= 0.0f) return;

        constexpr float kMinVisibleHeight = 1.0f;

        for (size_t i = 0; i < barCount; ++i) {
            const float normalizedHeight = Sanitize::NormalizedFloat(spectrum[i]);
            const float height = normalizedHeight * bounds.height;

            if (height < kMinVisibleHeight) continue;

            const Rect barRect = {
                bounds.x + i * totalBarWidth + style.spacing * 0.5f,
                bounds.y + bounds.height - height,
                barWidth,
                height
            };

            DrawSingleBar(barRect, style, color);
        }
    }

    void SpectrumRenderer::DrawWaveform(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Color& color,
        float strokeWidth,
        bool mirror
    ) const
    {
        if (!m_geometryBuilder || !m_primitiveRenderer) return;
        if (!Validate::PointArray(std::vector<Point>(spectrum.size()), 2)) return;

        auto points = m_geometryBuilder->GenerateWaveformPoints(spectrum, bounds);
        if (points.empty()) return;

        m_primitiveRenderer->DrawPolyline(points, color, strokeWidth);

        if (!mirror) return;

        const float midline = bounds.y + bounds.height * 0.5f;

        for (auto& point : points) {
            point.y = 2.0f * midline - point.y;
        }

        Color mirrorColor = color;
        mirrorColor.a *= 0.6f;

        m_primitiveRenderer->DrawPolyline(points, mirrorColor, strokeWidth);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SpectrumRenderer::DrawSingleBar(
        const Rect& barRect,
        const BarStyle& style,
        const Color& color
    ) const
    {
        if (style.useGradient && Validate::GradientStops(style.gradientStops) && m_gradientRenderer) {
            m_gradientRenderer->DrawVerticalGradientBar(
                barRect,
                style.gradientStops,
                style.cornerRadius
            );
            return;
        }

        if (!m_primitiveRenderer) return;

        m_primitiveRenderer->DrawRoundedRectangle(
            barRect,
            style.cornerRadius,
            color,
            true
        );
    }

} // namespace Spectrum