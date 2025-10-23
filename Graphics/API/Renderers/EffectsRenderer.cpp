// EffectsRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the EffectsRenderer class. This file contains optimized
// implementations of visual effects that enhance rendered shapes.
//
// Implementation details:
// - Shadow: Renders callback twice (offset shadow + original)
// - Glow: Multiple layered radial gradients with decreasing opacity
// - Opacity layers: Uses D2D layer stack with proper cleanup
// - All state changes are RAII-protected for exception safety
// - Uses D2DHelpers utilities for sanitization and validation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "EffectsRenderer.h"
#include "Graphics/API/D2DHelpers.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;
    using namespace Helpers::Scopes;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    EffectsRenderer::EffectsRenderer()
        : m_renderTarget(nullptr), m_brush(nullptr)
    {
    }

    void EffectsRenderer::OnRenderTargetChanged(ID2D1RenderTarget* renderTarget)
    {
        m_renderTarget = renderTarget;
    }

    void EffectsRenderer::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        m_brush = nullptr;
    }

    void EffectsRenderer::SetSolidBrush(ID2D1SolidColorBrush* brush)
    {
        m_brush = brush;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Effect Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void EffectsRenderer::DrawWithShadow(
        std::function<void()> drawCallback, const Point& offset, float /*blur*/, const Color& shadowColor
    ) const
    {
        if (!RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (!drawCallback) return;

        const D2D1_COLOR_F originalColor = m_brush->GetColor();

        {
            ScopedTransform transform(m_renderTarget, D2D1::Matrix3x2F::Translation(offset.x, offset.y));
            m_brush->SetColor(ToD2DColor(shadowColor));
            drawCallback();
        }

        m_brush->SetColor(originalColor);
        drawCallback();
    }

    void EffectsRenderer::DrawGlow(
        const Point& center, float radius, const Color& glowColor, float intensity
    ) const
    {
        if (!m_renderTarget) return;
        if (!PositiveRadius(radius)) return;

        const float sanitizedIntensity = NormalizedFloat(intensity);
        constexpr int kMinLayers = 3;
        constexpr int kMaxLayers = 10;
        constexpr int kDefaultLayers = 5;
        const int layers = std::clamp(kDefaultLayers, kMinLayers, kMaxLayers);

        for (int i = 0; i < layers; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(layers - 1);

            Color layerColor = glowColor;
            layerColor.a *= (1.0f - t) * 0.2f * sanitizedIntensity;

            const float currentRadius = radius * (1.0f + t * 0.5f);

            wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
            const Color transparentColor = { layerColor.r, layerColor.g, layerColor.b, 0.0f };
            const D2D1_GRADIENT_STOP stops[] = { { 0.0f, ToD2DColor(layerColor) }, { 1.0f, ToD2DColor(transparentColor) } };

            HRESULT hr = m_renderTarget->CreateGradientStopCollection(stops, 2, stopCollection.GetAddressOf());
            if (FAILED(hr)) continue;

            wrl::ComPtr<ID2D1RadialGradientBrush> gradientBrush;
            const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props = D2D1::RadialGradientBrushProperties(ToD2DPoint(center), D2D1::Point2F(0.0f, 0.0f), currentRadius, currentRadius);

            hr = m_renderTarget->CreateRadialGradientBrush(props, stopCollection.Get(), gradientBrush.GetAddressOf());
            if (FAILED(hr)) continue;

            const D2D1_ELLIPSE ellipse = { ToD2DPoint(center), currentRadius, currentRadius };
            m_renderTarget->FillEllipse(&ellipse, gradientBrush.Get());
        }
    }

    void EffectsRenderer::BeginOpacityLayer(float opacity) const
    {
        if (m_renderTarget) {
            wrl::ComPtr<ID2D1Layer> layer;
            if (SUCCEEDED(m_renderTarget->CreateLayer(nullptr, layer.GetAddressOf()))) {
                D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
                params.opacity = NormalizedFloat(opacity);
                m_renderTarget->PushLayer(&params, layer.Get());
            }
        }
    }

    void EffectsRenderer::EndOpacityLayer() const
    {
        if (m_renderTarget) {
            m_renderTarget->PopLayer();
        }
    }

    void EffectsRenderer::PushClipRect(const Rect& rect) const
    {
        if (m_renderTarget) {
            m_renderTarget->PushAxisAlignedClip(ToD2DRect(rect), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }
    }

    void EffectsRenderer::PopClipRect() const
    {
        if (m_renderTarget) {
            m_renderTarget->PopAxisAlignedClip();
        }
    }

} // namespace Spectrum