#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_D2D_SCOPES_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_D2D_SCOPES_H

#include "Common/Types.h"
#include "TypeConversion.h"
#include "Sanitization.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <wrl/client.h>

namespace Spectrum::Helpers::Scopes {

    // RAII wrapper for transform state management
    class ScopedTransform final
    {
    public:
        ScopedTransform(ID2D1RenderTarget* renderTarget, const D2D1_MATRIX_3X2_F& transform)
            : m_renderTarget(renderTarget)
            , m_oldTransform(D2D1::Matrix3x2F::Identity())
            , m_transformApplied(false)
        {
            if (!m_renderTarget) return;

            m_renderTarget->GetTransform(&m_oldTransform);
            m_renderTarget->SetTransform(transform * m_oldTransform);
            m_transformApplied = true;
        }

        ~ScopedTransform()
        {
            if (m_renderTarget && m_transformApplied) {
                m_renderTarget->SetTransform(m_oldTransform);
            }
        }

        ScopedTransform(const ScopedTransform&) = delete;
        ScopedTransform& operator=(const ScopedTransform&) = delete;

    private:
        ID2D1RenderTarget* m_renderTarget;
        D2D1_MATRIX_3X2_F m_oldTransform;
        bool m_transformApplied;
    };

    // RAII wrapper for opacity layer management
    class ScopedOpacityLayer final
    {
    public:
        ScopedOpacityLayer(ID2D1RenderTarget* renderTarget, float opacity)
            : m_renderTarget(renderTarget)
            , m_layerPushed(false)
        {
            if (!m_renderTarget) return;

            Microsoft::WRL::ComPtr<ID2D1Layer> layer;
            if (FAILED(m_renderTarget->CreateLayer(nullptr, layer.GetAddressOf()))) return;

            D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
            params.opacity = Sanitize::NormalizedFloat(opacity);

            m_renderTarget->PushLayer(&params, layer.Get());
            m_layerPushed = true;
        }

        ~ScopedOpacityLayer()
        {
            if (m_renderTarget && m_layerPushed) {
                m_renderTarget->PopLayer();
            }
        }

        ScopedOpacityLayer(const ScopedOpacityLayer&) = delete;
        ScopedOpacityLayer& operator=(const ScopedOpacityLayer&) = delete;

    private:
        ID2D1RenderTarget* m_renderTarget;
        bool m_layerPushed;
    };

    // RAII wrapper for clip rectangle management
    class ScopedClipRect final
    {
    public:
        ScopedClipRect(ID2D1RenderTarget* renderTarget, const Rect& rect)
            : m_renderTarget(renderTarget)
            , m_clipPushed(false)
        {
            if (!m_renderTarget) return;

            m_renderTarget->PushAxisAlignedClip(
                TypeConversion::ToD2DRect(rect),
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
            );
            m_clipPushed = true;
        }

        ~ScopedClipRect()
        {
            if (m_renderTarget && m_clipPushed) {
                m_renderTarget->PopAxisAlignedClip();
            }
        }

        ScopedClipRect(const ScopedClipRect&) = delete;
        ScopedClipRect& operator=(const ScopedClipRect&) = delete;

    private:
        ID2D1RenderTarget* m_renderTarget;
        bool m_clipPushed;
    };

} // namespace Spectrum::Helpers::Scopes

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_D2D_SCOPES_H