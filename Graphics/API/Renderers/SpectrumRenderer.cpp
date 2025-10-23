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

#include "Graphics/API/Renderers/SpectrumRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Renderers/PrimitiveRenderer.h"
#include "Graphics/API/Core/GeometryBuilder.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Brushes/GradientStop.h"

namespace Spectrum {

    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    SpectrumRenderer::SpectrumRenderer(
        PrimitiveRenderer* primitiveRenderer,
        GeometryBuilder* geometryBuilder
    )
        : m_primitiveRenderer(primitiveRenderer)
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
            const float normalizedHeight = NormalizedFloat(spectrum[i]);
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
        const Paint& paint,
        bool mirror
    ) const
    {
        if (!m_geometryBuilder || !m_primitiveRenderer) return;
        if (spectrum.size() < 2) return;

        auto points = m_geometryBuilder->GenerateWaveformPoints(spectrum, bounds);
        if (points.empty()) return;

        m_primitiveRenderer->DrawPolyline(
            points,
            paint
        );

        if (!mirror) return;

        const float midline = bounds.y + bounds.height * 0.5f;

        for (auto& point : points) {
            point.y = 2.0f * midline - point.y;
        }

        m_primitiveRenderer->DrawPolyline(
            points,
            paint.WithAlpha(paint.GetAlpha() * 0.6f)
        );
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
        if (!m_primitiveRenderer) {
            return;
        }

        if (style.useGradient && !style.gradientStops.empty()) {
            std::vector<GradientStop> stops;
            stops.reserve(style.gradientStops.size());
            for (const auto& d2dStop : style.gradientStops) {
                stops.push_back({
                    d2dStop.position,
                    Color(d2dStop.color.r, d2dStop.color.g, d2dStop.color.b, d2dStop.color.a)
                    });
            }

            Paint gradPaint = Paint::LinearGradient(
                { barRect.x, barRect.y },
                { barRect.x, barRect.GetBottom() },
                stops
            );

            m_primitiveRenderer->DrawRoundedRectangle(
                barRect,
                style.cornerRadius,
                gradPaint
            );
        }
        else {
            m_primitiveRenderer->DrawRoundedRectangle(
                barRect,
                style.cornerRadius,
                Paint::Fill(color)
            );
        }
    }

} // namespace Spectrum