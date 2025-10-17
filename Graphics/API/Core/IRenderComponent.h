#ifndef SPECTRUM_CPP_GRAPHICS_API_CORE_I_RENDER_COMPONENT_H
#define SPECTRUM_CPP_GRAPHICS_API_CORE_I_RENDER_COMPONENT_H

#include <d2d1.h>

namespace Spectrum {

    class IRenderComponent {
    public:
        virtual ~IRenderComponent() = default;
        virtual void OnRenderTargetChanged(ID2D1RenderTarget* renderTarget) = 0;
        virtual void OnDeviceLost() = 0;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GRAPHICS_API_CORE_I_RENDER_COMPONENT_H