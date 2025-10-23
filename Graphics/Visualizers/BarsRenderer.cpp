// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the BarsRenderer for classic vertical bar visualization.
//
// Implementation details:
// - Each bar height calculated from magnitude
// - Optional shadow rendered before main bar
// - Optional highlight rendered after main bar
// - Rounded corners controlled by quality setting
// - Brightness increases with magnitude for visual impact
// - Uses GeometryHelpers for all geometric operations
//
// Rendering pipeline:
// 1. Calculate bar layout (spacing and width)
// 2. For each visible bar:
//    a. Calculate bar color from magnitude
//    b. Render shadow layer (if enabled)
//    c. Render main bar shape
//    d. Render highlight overlay (if enabled)
//
// Visual design:
// - Bars grow from bottom to top
// - Highlight appears on top 20% of bar
// - Shadow offset slightly to bottom-right
// - Color brightness range: 70% to 130%
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/BarsRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kHeightScale = 0.9f;
        constexpr float kMinVisibleHeight = 1.0f;

        constexpr float kShadowOffsetX = 2.0f;
        constexpr float kShadowOffsetY = 2.0f;
        constexpr float kShadowBlurRadius = 0.0f;
        constexpr float kShadowAlpha = 0.3f;

        constexpr float kHighlightMargin = 2.0f;
        constexpr float kHighlightMaxHeight = 10.0f;
        constexpr float kHighlightHeightRatio = 0.2f;
        constexpr float kHighlightAlpha = 0.2f;

        constexpr float kBrightnessMin = 0.7f;
        constexpr float kBrightnessRange = 0.6f;

        constexpr float kSpacingDivisor = 2.0f;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    BarsRenderer::BarsRenderer()
    {
        m_primaryColor = Color::FromRGB(33, 150, 243);
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BarsRenderer::UpdateSettings()
    {
        m_settings = QualityPresets::Get<BarsRenderer>(m_quality);
    }

    void BarsRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        const size_t barCount = spectrum.size();
        const auto layout = RenderUtils::ComputeBarLayout(
            barCount,
            m_settings.barSpacing,
            m_width
        );

        if (layout.barWidth <= 0.0f) return;

        for (size_t i = 0; i < barCount; ++i) {
            const float magnitude = NormalizedFloat(spectrum[i]);
            const float height = RenderUtils::MagnitudeToHeight(
                magnitude,
                m_height,
                kHeightScale
            );

            if (!IsBarVisible(height)) continue;

            const Rect rect = CalculateBarRect(i, height, layout);

            RenderBar(canvas, rect, magnitude);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BarsRenderer::RenderBar(
        Canvas& canvas,
        const Rect& rect,
        float magnitude
    ) const
    {
        const Color barColor = CalculateBarColor(magnitude);
        RenderBarWithEffects(canvas, rect, barColor);
        RenderHighlight(canvas, rect, magnitude);
    }

    void BarsRenderer::RenderBarWithEffects(
        Canvas& canvas,
        const Rect& rect,
        const Color& color
    ) const
    {
        auto drawCall = [this, &canvas, &rect, &color]() {
            RenderMainBar(canvas, rect, color);
            };

        if (m_settings.useShadow) {
            canvas.DrawWithShadow(
                drawCall,
                { kShadowOffsetX, kShadowOffsetY },
                kShadowBlurRadius,
                Color(0.0f, 0.0f, 0.0f, kShadowAlpha)
            );
            drawCall();
        }
        else {
            drawCall();
        }
    }

    void BarsRenderer::RenderMainBar(
        Canvas& canvas,
        const Rect& rect,
        const Color& color
    ) const
    {
        const Paint paint = Paint::Fill(color);

        if (m_settings.cornerRadius > 0.0f) {
            canvas.DrawRoundedRectangle(
                rect,
                m_settings.cornerRadius,
                paint
            );
        }
        else {
            canvas.DrawRectangle(rect, paint);
        }
    }

    void BarsRenderer::RenderHighlight(
        Canvas& canvas,
        const Rect& rect,
        float magnitude
    ) const
    {
        if (!m_settings.useHighlight) return;

        const Rect highlightRect = CalculateHighlightRect(rect);

        if (!IsHighlightVisible(highlightRect)) return;

        const Color highlightColor = CalculateHighlightColor(magnitude);
        canvas.DrawRectangle(highlightRect, Paint::Fill(highlightColor));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Rect BarsRenderer::CalculateBarRect(
        size_t index,
        float height,
        const RenderUtils::BarLayout& layout
    ) const
    {
        return {
            index * layout.totalBarWidth + layout.spacing / kSpacingDivisor,
            static_cast<float>(m_height) - height,
            layout.barWidth,
            height
        };
    }

    Rect BarsRenderer::CalculateHighlightRect(const Rect& barRect) const
    {
        return Deflate(
            {
                barRect.x,
                barRect.y,
                barRect.width,
                std::min(kHighlightMaxHeight, barRect.height * kHighlightHeightRatio)
            },
            -kHighlightMargin
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color BarsRenderer::CalculateBarColor(float magnitude) const
    {
        return Helpers::Color::AdjustBrightness(
            m_primaryColor,
            kBrightnessMin + kBrightnessRange * magnitude
        );
    }

    Color BarsRenderer::CalculateHighlightColor(float magnitude) const
    {
        return Color(1.0f, 1.0f, 1.0f, kHighlightAlpha * magnitude);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool BarsRenderer::IsBarVisible(float height) const
    {
        return height >= kMinVisibleHeight;
    }

    bool BarsRenderer::IsHighlightVisible(const Rect& rect) const
    {
        return IsValid(rect);
    }

} // namespace Spectrum