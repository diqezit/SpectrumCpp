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
#include <vector>

namespace Spectrum {

    class PrimitiveRenderer;
    class GeometryBuilder;
    class Paint;
    struct GradientStop;

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
        // Validation Helpers (DRY)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ValidateRendererDependencies() const noexcept;
        [[nodiscard]] bool ValidateSpectrumData(const SpectrumData& spectrum) const noexcept;
        [[nodiscard]] bool ValidateWaveformData(const SpectrumData& spectrum) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Bar Rendering Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct BarDimensions {
            float totalWidth;
            float barWidth;
        };

        [[nodiscard]] BarDimensions CalculateBarDimensions(
            const Rect& bounds,
            size_t barCount,
            float spacing
        ) const noexcept;

        [[nodiscard]] bool IsBarVisible(float barWidth) const noexcept;
        [[nodiscard]] bool IsBarHeightVisible(float height) const noexcept;

        [[nodiscard]] Rect CalculateBarRect(
            const Rect& bounds,
            const BarDimensions& dims,
            size_t index,
            float height,
            float spacing
        ) const noexcept;

        void DrawBar(
            const Rect& barRect,
            const BarStyle& style,
            const Color& color
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Paint Creation Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Paint CreateBarPaint(
            const Rect& barRect,
            const BarStyle& style,
            const Color& color
        ) const;

        [[nodiscard]] Paint CreateGradientPaint(
            const Rect& barRect,
            const BarStyle& style
        ) const;

        [[nodiscard]] Paint CreateSolidPaint(const Color& color) const;

        [[nodiscard]] std::vector<GradientStop> ConvertGradientStops(
            const std::vector<D2D1_GRADIENT_STOP>& d2dStops
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Waveform Rendering Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawWaveformLine(
            const std::vector<Point>& points,
            const Paint& paint
        ) const;

        void DrawMirroredWaveform(
            std::vector<Point> points,
            const Rect& bounds,
            const Paint& paint
        ) const;

        void MirrorPointsVertically(
            std::vector<Point>& points,
            float centerY
        ) const;

        [[nodiscard]] float CalculateMirrorOpacity(float originalOpacity) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        PrimitiveRenderer* m_primitiveRenderer;
        GeometryBuilder* m_geometryBuilder;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SPECTRUM_RENDERER_H