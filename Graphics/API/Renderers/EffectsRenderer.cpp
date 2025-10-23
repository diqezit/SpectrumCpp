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
#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Rendering;
    using namespace Helpers::Scopes;

    namespace {
        constexpr int kMinGlowLayers = 3;
        constexpr int kMaxGlowLayers = 10;
        constexpr int kDefaultGlowLayers = 5;
        constexpr float kGlowLayerOpacityFactor = 0.2f;
        constexpr float kGlowRadiusMultiplier = 0.5f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    EffectsRenderer::EffectsRenderer()
        : m_brush(nullptr)
    {
    }

    void EffectsRenderer::OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget)
    {
        m_renderTarget = renderTarget;
    }

    void EffectsRenderer::OnDeviceLost()
    {
        m_renderTarget.Reset();
        m_brush = nullptr;
        m_activeLayer.Reset();
    }

    void EffectsRenderer::SetSolidBrush(ID2D1SolidColorBrush* brush)
    {
        m_brush = brush;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Glow Effect Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    int EffectsRenderer::CalculateGlowLayerCount() const noexcept
    {
        return std::clamp(
            kDefaultGlowLayers,
            kMinGlowLayers,
            kMaxGlowLayers
        );
    }

    wrl::ComPtr<ID2D1GradientStopCollection> EffectsRenderer::CreateGlowGradientStops(
        const Color& glowColor,
        float layerT,
        float intensity
    ) const {
        if (!m_renderTarget) {
            return nullptr;
        }

        Color layerColor = glowColor;
        layerColor.a *= (1.0f - layerT) * kGlowLayerOpacityFactor * intensity;

        const Color transparentColor = {
            layerColor.r,
            layerColor.g,
            layerColor.b,
            0.0f
        };

        const D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, ToD2DColor(layerColor) },
            { 1.0f, ToD2DColor(transparentColor) }
        };

        return BrushManager::CreateGradientStops(m_renderTarget.Get(),
            std::vector<D2D1_GRADIENT_STOP>(stops, stops + 2));
    }

    wrl::ComPtr<ID2D1RadialGradientBrush> EffectsRenderer::CreateGlowBrush(
        const Point& center,
        float radius,
        ID2D1GradientStopCollection* stopCollection
    ) const {
        return BrushManager::CreateRadialGradientBrush(
            m_renderTarget.Get(), center, radius, radius, stopCollection);
    }

    void EffectsRenderer::DrawGlowLayer(
        const Point& center,
        const Color& glowColor,
        float radius,
        float layerT,
        float intensity
    ) const {
        const float currentRadius = radius * (1.0f + layerT * kGlowRadiusMultiplier);

        auto stopCollection = CreateGlowGradientStops(glowColor, layerT, intensity);
        if (!stopCollection) return;

        auto gradientBrush = CreateGlowBrush(center, currentRadius, stopCollection.Get());
        if (!gradientBrush) return;

        const D2D1_ELLIPSE ellipse = {
            ToD2DPoint(center),
            currentRadius,
            currentRadius
        };

        m_renderTarget->FillEllipse(&ellipse, gradientBrush.Get());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Effect Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void EffectsRenderer::DrawWithShadow(
        std::function<void()> drawCallback,
        const Point& offset,
        float /*blur*/,
        const Color& shadowColor
    ) const {
        if (!RenderValidation::ValidateRenderingContext(m_renderTarget.Get(), m_brush)) return;
        if (!drawCallback) return;

        RenderPatterns::DrawWithShadow(
            m_renderTarget.Get(),
            drawCallback,
            offset,
            shadowColor,
            m_brush
        );
    }

    void EffectsRenderer::DrawGlow(
        const Point& center,
        float radius,
        const Color& glowColor,
        float intensity
    ) const {
        if (!RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;
        if (!Helpers::Validate::PositiveRadius(radius)) return;

        const float sanitizedIntensity = Helpers::Sanitize::NormalizedFloat(intensity);
        const int layers = CalculateGlowLayerCount();

        for (int i = 0; i < layers; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(layers - 1);
            DrawGlowLayer(center, glowColor, radius, t, sanitizedIntensity);
        }
    }

    void EffectsRenderer::BeginOpacityLayer(float opacity) const
    {
        if (m_activeLayer || !RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) return;

        HRESULT hr = m_renderTarget->CreateLayer(nullptr, m_activeLayer.ReleaseAndGetAddressOf());
        if (SUCCEEDED(hr) && m_activeLayer) {
            D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
            params.opacity = Helpers::Sanitize::NormalizedFloat(opacity);
            m_renderTarget->PushLayer(&params, m_activeLayer.Get());
        }
        else {
            m_activeLayer.Reset();
        }
    }

    void EffectsRenderer::EndOpacityLayer() const
    {
        if (m_activeLayer && RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) {
            m_renderTarget->PopLayer();
            m_activeLayer.Reset();
        }
    }

    void EffectsRenderer::PushClipRect(const Rect& rect) const
    {
        if (RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) {
            m_renderTarget->PushAxisAlignedClip(
                ToD2DRect(rect),
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
            );
        }
    }

    void EffectsRenderer::PopClipRect() const
    {
        if (RenderValidation::ValidateRenderTarget(m_renderTarget.Get())) {
            m_renderTarget->PopAxisAlignedClip();
        }
    }

} // namespace Spectrum