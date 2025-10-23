// EffectsRenderer.h
#ifndef SPECTRUM_CPP_EFFECTS_RENDERER_H
#define SPECTRUM_CPP_EFFECTS_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the EffectsRenderer, a class for applying visual effects like
// shadows, glows, and opacity layers. It encapsulates the logic for D2D
// layers and custom effect implementations.
//
// Key responsibilities:
// - Shadow rendering through offset drawing (performance-optimized)
// - Glow effects via layered radial gradients
// - Opacity layer management with automatic cleanup
// - Clip rectangle management for masking
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Uses RAII wrappers from D2DHelpers for state management
// - Non-owning pointers to render resources (lifetime managed externally)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <functional>

namespace Spectrum {

    class EffectsRenderer final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        EffectsRenderer();
        EffectsRenderer(const EffectsRenderer&) = delete;
        EffectsRenderer& operator=(const EffectsRenderer&) = delete;

        // IRenderComponent implementation
        void OnRenderTargetChanged(ID2D1RenderTarget* renderTarget) override;
        void OnDeviceLost() override;

        void SetSolidBrush(ID2D1SolidColorBrush* brush);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Effect Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawWithShadow(std::function<void()> drawCallback, const Point& offset, float blur, const Color& shadowColor) const;
        void DrawGlow(const Point& center, float radius, const Color& glowColor, float intensity = 1.0f) const;
        void BeginOpacityLayer(float opacity) const;
        void EndOpacityLayer() const;
        void PushClipRect(const Rect& rect) const;
        void PopClipRect() const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1RenderTarget* m_renderTarget;
        ID2D1SolidColorBrush* m_brush;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_EFFECTS_RENDERER_H