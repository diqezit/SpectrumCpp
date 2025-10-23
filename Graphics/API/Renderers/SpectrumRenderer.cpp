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

    namespace {
        constexpr float kMinVisibleBarHeight = 1.0f;
        constexpr float kMirrorOpacityFactor = 0.6f;
        constexpr size_t kMinWaveformPoints = 2;
    }

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
    // Validation Helpers (DRY)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool SpectrumRenderer::ValidateRendererDependencies() const noexcept
    {
        return m_primitiveRenderer != nullptr && m_geometryBuilder != nullptr;
    }

    bool SpectrumRenderer::ValidateSpectrumData(
        const SpectrumData& spectrum
    ) const noexcept {
        return !spectrum.empty();
    }

    bool SpectrumRenderer::ValidateWaveformData(
        const SpectrumData& spectrum
    ) const noexcept {
        return spectrum.size() >= kMinWaveformPoints;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Bar Rendering Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    SpectrumRenderer::BarDimensions SpectrumRenderer::CalculateBarDimensions(
        const Rect& bounds,
        size_t barCount,
        float spacing
    ) const noexcept {
        const float totalWidth = bounds.width / static_cast<float>(barCount);
        const float barWidth = totalWidth - spacing;

        return { totalWidth, barWidth };
    }

    bool SpectrumRenderer::IsBarVisible(float barWidth) const noexcept
    {
        return barWidth > 0.0f;
    }

    bool SpectrumRenderer::IsBarHeightVisible(float height) const noexcept
    {
        return height >= kMinVisibleBarHeight;
    }

    Rect SpectrumRenderer::CalculateBarRect(
        const Rect& bounds,
        const BarDimensions& dims,
        size_t index,
        float height,
        float spacing
    ) const noexcept {
        return {
            bounds.x + index * dims.totalWidth + spacing * 0.5f,
            bounds.y + bounds.height - height,
            dims.barWidth,
            height
        };
    }

    void SpectrumRenderer::DrawBar(
        const Rect& barRect,
        const BarStyle& style,
        const Color& color
    ) const {
        if (!m_primitiveRenderer) return;

        Paint paint = CreateBarPaint(barRect, style, color);

        m_primitiveRenderer->DrawRoundedRectangle(
            barRect,
            style.cornerRadius,
            paint
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paint Creation Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Paint SpectrumRenderer::CreateBarPaint(
        const Rect& barRect,
        const BarStyle& style,
        const Color& color
    ) const {
        if (style.useGradient && !style.gradientStops.empty()) {
            return CreateGradientPaint(barRect, style);
        }
        return CreateSolidPaint(color);
    }

    Paint SpectrumRenderer::CreateGradientPaint(
        const Rect& barRect,
        const BarStyle& style
    ) const {
        std::vector<GradientStop> stops = ConvertGradientStops(style.gradientStops);

        return Paint::LinearGradient(
            { barRect.x, barRect.y },
            { barRect.x, barRect.GetBottom() },
            stops
        );
    }

    Paint SpectrumRenderer::CreateSolidPaint(const Color& color) const
    {
        return Paint::Fill(color);
    }

    std::vector<GradientStop> SpectrumRenderer::ConvertGradientStops(
        const std::vector<D2D1_GRADIENT_STOP>& d2dStops
    ) const {
        std::vector<GradientStop> stops;
        stops.reserve(d2dStops.size());

        for (const auto& d2dStop : d2dStops) {
            stops.push_back({
                d2dStop.position,
                Color(
                    d2dStop.color.r,
                    d2dStop.color.g,
                    d2dStop.color.b,
                    d2dStop.color.a
                )
                });
        }

        return stops;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Waveform Rendering Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SpectrumRenderer::DrawWaveformLine(
        const std::vector<Point>& points,
        const Paint& paint
    ) const {
        if (!m_primitiveRenderer || points.empty()) return;

        m_primitiveRenderer->DrawPolyline(points, paint);
    }

    void SpectrumRenderer::DrawMirroredWaveform(
        std::vector<Point> points,
        const Rect& bounds,
        const Paint& paint
    ) const {
        const float midline = bounds.y + bounds.height * 0.5f;

        MirrorPointsVertically(points, midline);

        const float mirrorOpacity = CalculateMirrorOpacity(paint.GetAlpha());
        Paint mirroredPaint = paint.WithAlpha(mirrorOpacity);

        DrawWaveformLine(points, mirroredPaint);
    }

    void SpectrumRenderer::MirrorPointsVertically(
        std::vector<Point>& points,
        float centerY
    ) const {
        for (auto& point : points) {
            point.y = 2.0f * centerY - point.y;
        }
    }

    float SpectrumRenderer::CalculateMirrorOpacity(float originalOpacity) const noexcept
    {
        return originalOpacity * kMirrorOpacityFactor;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Spectrum Visualization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void SpectrumRenderer::DrawSpectrumBars(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const BarStyle& style,
        const Color& color
    ) const {
        if (!ValidateSpectrumData(spectrum)) return;

        const size_t barCount = spectrum.size();
        const BarDimensions dims = CalculateBarDimensions(
            bounds,
            barCount,
            style.spacing
        );

        if (!IsBarVisible(dims.barWidth)) return;

        for (size_t i = 0; i < barCount; ++i) {
            const float normalizedHeight = NormalizedFloat(spectrum[i]);
            const float height = normalizedHeight * bounds.height;

            if (!IsBarHeightVisible(height)) continue;

            const Rect barRect = CalculateBarRect(
                bounds,
                dims,
                i,
                height,
                style.spacing
            );

            DrawBar(barRect, style, color);
        }
    }

    void SpectrumRenderer::DrawWaveform(
        const SpectrumData& spectrum,
        const Rect& bounds,
        const Paint& paint,
        bool mirror
    ) const {
        if (!ValidateRendererDependencies()) return;
        if (!ValidateWaveformData(spectrum)) return;

        auto points = m_geometryBuilder->GenerateWaveformPoints(spectrum, bounds);
        if (points.empty()) return;

        DrawWaveformLine(points, paint);

        if (mirror) {
            DrawMirroredWaveform(points, bounds, paint);
        }
    }

} // namespace Spectrum