#ifndef SPECTRUM_CPP_GRAPHICS_API_CORE_I_RENDER_COMPONENT_H
#define SPECTRUM_CPP_GRAPHICS_API_CORE_I_RENDER_COMPONENT_H

#include <d2d1.h>
#include <wrl/client.h>

namespace Spectrum {

    namespace wrl = Microsoft::WRL;

    class IRenderComponent {
    public:
        enum class Priority {
            High = 0,
            Normal = 1,
            Low = 2
        };

        virtual ~IRenderComponent() = default;
        virtual void OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget) = 0;
        virtual void OnDeviceLost() = 0;
        virtual Priority GetPriority() const { return Priority::Normal; }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GRAPHICS_API_CORE_I_RENDER_COMPONENT_H