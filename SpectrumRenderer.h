#ifndef SPECTRUM_CPP_SPECTRUM_RENDERER_H
#define SPECTRUM_CPP_SPECTRUM_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the SpectrumRenderer, a specialized component that
// combines other renderers to draw common spectrum visualizations like
// bars and waveforms. It acts as a composite renderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "SpectrumTypes.h"
#include "GeometryBuilder.h"
#include "GradientRenderer.h"
#include "PrimitiveRenderer.h"

namespace Spectrum {

    class SpectrumRenderer {
    public:
        SpectrumRenderer(
            PrimitiveRenderer* primitiveRenderer,
            GradientRenderer* gradientRenderer,
            GeometryBuilder* geometryBuilder
        );

        void DrawSpectrumBars(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const BarStyle& style,
            const Color& color
        );

        void DrawWaveform(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Color& color,
            float strokeWidth = 2.0f,
            bool mirror = false
        );

    private:
        void DrawSingleBar(
            const Rect& barRect,
            const BarStyle& style,
            const Color& color
        );

        PrimitiveRenderer* m_primitiveRenderer;
        GradientRenderer* m_gradientRenderer;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif