#ifndef SPECTRUM_CPP_SPECTRUM_RENDERER_H
#define SPECTRUM_CPP_SPECTRUM_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the SpectrumRenderer, a specialized component that combines
// primitive and gradient renderers to draw common spectrum visualizations
// like bars and waveforms. It acts as a composite renderer.
//
// Key responsibilities:
// - Spectrum bar rendering with configurable styles
// - Waveform visualization with optional mirroring
// - Integration of gradient and solid color rendering
// - Bar style management (gradient, corner radius, spacing)
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Delegates primitive drawing to PrimitiveRenderer
// - Delegates gradient drawing to ResourceCache via PrimitiveRenderer
// - Uses GeometryBuilder for waveform point generation
// - Non-owning pointers to dependencies (lifetime managed externally)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/SpectrumTypes.h"

namespace Spectrum {

    class PrimitiveRenderer;
    class GeometryBuilder;
    class Paint;

    class SpectrumRenderer final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        SpectrumRenderer(
            PrimitiveRenderer* primitiveRenderer,
            GeometryBuilder* geometryBuilder
        );

        SpectrumRenderer(const SpectrumRenderer&) = delete;
        SpectrumRenderer& operator=(const SpectrumRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Spectrum Visualization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawSpectrumBars(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const BarStyle& style,
            const Color& color
        ) const;

        void DrawWaveform(
            const SpectrumData& spectrum,
            const Rect& bounds,
            const Paint& paint,
            bool mirror = false
        ) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawSingleBar(
            const Rect& barRect,
            const BarStyle& style,
            const Color& color
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        PrimitiveRenderer* m_primitiveRenderer;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SPECTRUM_RENDERER_H