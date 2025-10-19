// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the EffectsRenderer class
// It provides methods for common visual effects that enhance the
// appearance of rendered shapes and visualizers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "EffectsRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "Random.h"

namespace Spectrum {

    namespace {
        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }

        inline D2D1_RECT_F ToD2DRect(const Rect& r) {
            return D2D1::RectF(r.x, r.y, r.GetRight(), r.GetBottom());
        }
    }

    EffectsRenderer::EffectsRenderer(
        ID2D1RenderTarget* renderTarget,
        ID2D1SolidColorBrush* brush
    )
        : m_renderTarget(renderTarget)
        , m_brush(brush)
    {
    }

    // renders a draw function twice, first offset and colored for the shadow
    // this fakes a shadow without needing complex D2D effects for better performance
    void EffectsRenderer::DrawWithShadow(
        std::function<void()> drawCallback,
        const Point& offset,
        float /*blur*/, // blur is currently unused, reserved for future D2D effect
        const Color& shadowColor
    ) {
        if (!m_renderTarget || !m_brush) {
            return;
        }

        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);

        // draw shadow first, offset from original position
        D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(
            offset.x,
            offset.y
        );
        m_renderTarget->SetTransform(translation * oldTransform);

        D2D1_COLOR_F oldColor = m_brush->GetColor();
        m_brush->SetColor(ToD2DColor(shadowColor));
        drawCallback();

        // restore state and draw main object on top
        m_brush->SetColor(oldColor);
        m_renderTarget->SetTransform(oldTransform);
        drawCallback();
    }

    // simulates glow by drawing multiple stacked, fading radial gradients
    // this avoids using expensive D2D bitmap effects
    void EffectsRenderer::DrawGlow(
        const Point& center,
        float radius,
        const Color& glowColor,
        float intensity
    ) {
        if (!m_renderTarget || !m_brush) {
            return;
        }

        // more layers create a smoother but more expensive glow
        const int layers = 5;

        for (int i = 0; i < layers; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(layers - 1);
            Color c = glowColor;
            // outer layers are more transparent
            c.a *= (1.0f - t) * 0.2f * intensity;
            // outer layers are larger
            float currentRadius = radius * (1.0f + t * 0.5f);

            wrl::ComPtr<ID2D1GradientStopCollection> stopCollection;
            Color transparentColor = { c.r, c.g, c.b, 0.0f };

            D2D1_GRADIENT_STOP stops[] = {
                { 0.0f, ToD2DColor(c) },
                { 1.0f, ToD2DColor(transparentColor) }
            };

            if (SUCCEEDED(m_renderTarget->CreateGradientStopCollection(
                stops,
                2,
                stopCollection.GetAddressOf()
            ))) {
                wrl::ComPtr<ID2D1RadialGradientBrush> gradientBrush;
                D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props =
                    D2D1::RadialGradientBrushProperties(
                        ToD2DPoint(center),
                        {},
                        currentRadius,
                        currentRadius
                    );

                if (SUCCEEDED(m_renderTarget->CreateRadialGradientBrush(
                    props,
                    stopCollection.Get(),
                    gradientBrush.GetAddressOf()
                ))) {
                    D2D1_ELLIPSE ellipse = {};
                    ellipse.point = ToD2DPoint(center);
                    ellipse.radiusX = currentRadius;
                    ellipse.radiusY = currentRadius;

                    m_renderTarget->FillEllipse(&ellipse, gradientBrush.Get());
                }
            }
        }
    }

    // user expects some elements to fade in/out
    // layers allow drawing group of objects with shared opacity
    void EffectsRenderer::BeginOpacityLayer(float opacity) {
        if (!m_renderTarget) {
            return;
        }

        wrl::ComPtr<ID2D1Layer> layer;
        if (SUCCEEDED(m_renderTarget->CreateLayer(nullptr, layer.GetAddressOf()))) {
            m_layerStack.push(layer);

            D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
            params.opacity = opacity;

            m_renderTarget->PushLayer(&params, layer.Get());
        }
    }

    void EffectsRenderer::EndOpacityLayer() {
        if (m_renderTarget && !m_layerStack.empty()) {
            m_renderTarget->PopLayer();
            m_layerStack.pop();
        }
    }

    // restricts drawing to a rectangular area
    // useful for UI panels or masking effects
    void EffectsRenderer::PushClipRect(const Rect& rect) {
        if (m_renderTarget) {
            m_renderTarget->PushAxisAlignedClip(
                ToD2DRect(rect),
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
            );
        }
    }

    void EffectsRenderer::PopClipRect() {
        if (m_renderTarget) {
            m_renderTarget->PopAxisAlignedClip();
        }
    }

    // when render target is recreated, layer stack must be cleared
    // otherwise old layers could be applied to new target
    void EffectsRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;

        while (!m_layerStack.empty()) {
            m_layerStack.pop();
        }
    }

} // namespace Spectrum