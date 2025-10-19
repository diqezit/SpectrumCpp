#ifndef SPECTRUM_CPP_EFFECTS_RENDERER_H
#define SPECTRUM_CPP_EFFECTS_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the EffectsRenderer, a class for applying visual
// effects like shadows, glows, and opacity layers. It encapsulates
// the logic for D2D layers and custom effect implementations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <functional>
#include <stack>

namespace Spectrum {

    class EffectsRenderer {
    public:
        EffectsRenderer(
            ID2D1RenderTarget* renderTarget,
            ID2D1SolidColorBrush* brush
        );

        void DrawWithShadow(
            std::function<void()> drawCallback,
            const Point& offset,
            float blur,
            const Color& shadowColor
        );

        void DrawGlow(
            const Point& center,
            float radius,
            const Color& glowColor,
            float intensity = 1.0f
        );

        void BeginOpacityLayer(float opacity);
        void EndOpacityLayer();

        void PushClipRect(const Rect& rect);
        void PopClipRect();

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        ID2D1RenderTarget* m_renderTarget;
        ID2D1SolidColorBrush* m_brush;
        std::stack<wrl::ComPtr<ID2D1Layer>> m_layerStack;
    };

} // namespace Spectrum

#endif