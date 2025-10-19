// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the SpectrumRenderer class
// It contains high-level logic for drawing spectrum data, delegating the
// actual drawing of primitives and gradients to other components
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "SpectrumRenderer.h"

namespace Spectrum {

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

    void SpectrumRenderer::DrawSpectrumBars(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const BarStyle& style,
        const Color& color
    ) {
        if (spectrum.empty()) {
            return;
        }

        size_t n = spectrum.size();
        if (n == 0) {
            return;
        }

        float totalBarWidth = bounds.width / static_cast<float>(n);
        float barWidth = totalBarWidth - style.spacing;

        // do not render if bars are too thin to be visible
        if (barWidth <= 0) {
            return;
        }

        for (size_t i = 0; i < n; ++i) {
            float h = spectrum[i] * bounds.height;
            // skip drawing bars that are too short to be seen
            if (h < 1.0f) {
                continue;
            }

            Rect barRect = {
                bounds.x + i * totalBarWidth + style.spacing / 2.0f,
                bounds.y + bounds.height - h,
                barWidth,
                h
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
    ) {
        if (spectrum.size() < 2 || !m_geometryBuilder || !m_primitiveRenderer) {
            return;
        }

        auto points = m_geometryBuilder->GenerateWaveformPoints(spectrum, bounds);
        m_primitiveRenderer->DrawPolyline(points, color, strokeWidth);

        // mirrored waveform gives a classic audio visualizer look
        if (mirror) {
            float midline = bounds.y + bounds.height * 0.5f;

            for (auto& p : points) {
                p.y = 2 * midline - p.y;
            }

            // make mirror slightly transparent for depth
            Color mirrorColor = color;
            mirrorColor.a *= 0.6f;
            m_primitiveRenderer->DrawPolyline(points, mirrorColor, strokeWidth);
        }
    }

    // chooses between gradient or solid color based on style settings
    void SpectrumRenderer::DrawSingleBar(
        const Rect& barRect,
        const BarStyle& style,
        const Color& color
    ) {
        if (style.useGradient && !style.gradientStops.empty() && m_gradientRenderer) {
            m_gradientRenderer->DrawVerticalGradientBar(
                barRect,
                style.gradientStops,
                style.cornerRadius
            );
        }
        else if (m_primitiveRenderer) {
            m_primitiveRenderer->DrawRoundedRectangle(
                barRect,
                style.cornerRadius,
                color,
                true
            );
        }
    }

} // namespace Spectrum