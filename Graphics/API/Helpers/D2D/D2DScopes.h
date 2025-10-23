#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_D2D_SCOPES_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_D2D_SCOPES_H

#include "Common/Types.h"
#include "Graphics/API/Helpers/Conversion/TypeConversion.h"
#include "Graphics/API/Helpers/Core/Sanitization.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <wrl/client.h>

namespace Spectrum::Helpers::Scopes {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Common validation helpers (DRY)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Detail {

        inline bool IsValidRenderTarget(ID2D1RenderTarget* renderTarget) noexcept
        {
            return renderTarget != nullptr;
        }

        inline bool ShouldRevertOperation(ID2D1RenderTarget* renderTarget, bool operationApplied) noexcept
        {
            return renderTarget && operationApplied;
        }

    } // namespace Detail

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RAII wrapper for transform state management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class ScopedTransform final
    {
    public:
        ScopedTransform(ID2D1RenderTarget* renderTarget, const D2D1_MATRIX_3X2_F& transform)
            : m_renderTarget(renderTarget)
            , m_oldTransform(D2D1::Matrix3x2F::Identity())
            , m_transformApplied(false)
        {
            if (!Detail::IsValidRenderTarget(m_renderTarget)) return;

            ApplyTransform(transform);
        }

        ~ScopedTransform()
        {
            RevertTransform();
        }

        ScopedTransform(const ScopedTransform&) = delete;
        ScopedTransform& operator=(const ScopedTransform&) = delete;

    private:
        void ApplyTransform(const D2D1_MATRIX_3X2_F& transform)
        {
            m_renderTarget->GetTransform(&m_oldTransform);
            m_renderTarget->SetTransform(transform * m_oldTransform);
            m_transformApplied = true;
        }

        void RevertTransform() noexcept
        {
            if (Detail::ShouldRevertOperation(m_renderTarget, m_transformApplied)) {
                m_renderTarget->SetTransform(m_oldTransform);
            }
        }

        ID2D1RenderTarget* m_renderTarget;
        D2D1_MATRIX_3X2_F m_oldTransform;
        bool m_transformApplied;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RAII wrapper for opacity layer management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class ScopedOpacityLayer final
    {
    public:
        ScopedOpacityLayer(ID2D1RenderTarget* renderTarget, float opacity)
            : m_renderTarget(renderTarget)
            , m_layerPushed(false)
        {
            if (!Detail::IsValidRenderTarget(m_renderTarget)) return;

            PushOpacityLayer(opacity);
        }

        ~ScopedOpacityLayer()
        {
            PopOpacityLayer();
        }

        ScopedOpacityLayer(const ScopedOpacityLayer&) = delete;
        ScopedOpacityLayer& operator=(const ScopedOpacityLayer&) = delete;

    private:
        void PushOpacityLayer(float opacity)
        {
            Microsoft::WRL::ComPtr<ID2D1Layer> layer;

            if (!TryCreateLayer(layer)) return;

            PushLayerWithOpacity(layer.Get(), opacity);
        }

        bool TryCreateLayer(Microsoft::WRL::ComPtr<ID2D1Layer>& layer)
        {
            HRESULT hr = m_renderTarget->CreateLayer(nullptr, layer.GetAddressOf());
            if (FAILED(hr)) {
                LogLayerCreationFailure(hr);
                return false;
            }
            return true;
        }

        void PushLayerWithOpacity(ID2D1Layer* layer, float opacity)
        {
            D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
            params.opacity = Sanitize::NormalizedFloat(opacity);

            m_renderTarget->PushLayer(&params, layer);
            m_layerPushed = true;
        }

        void PopOpacityLayer() noexcept
        {
            if (Detail::ShouldRevertOperation(m_renderTarget, m_layerPushed)) {
                m_renderTarget->PopLayer();
            }
        }

        static void LogLayerCreationFailure(HRESULT hr)
        {
            UNREFERENCED_PARAMETER(hr);
            LOG_WARNING("CreateLayer failed: 0x" << std::hex << hr << " - opacity layer disabled");
        }

        ID2D1RenderTarget* m_renderTarget;
        bool m_layerPushed;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RAII wrapper for clip rectangle management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class ScopedClipRect final
    {
    public:
        ScopedClipRect(ID2D1RenderTarget* renderTarget, const Rect& rect)
            : m_renderTarget(renderTarget)
            , m_clipPushed(false)
        {
            if (!Detail::IsValidRenderTarget(m_renderTarget)) return;

            PushClipRect(rect);
        }

        ~ScopedClipRect()
        {
            PopClipRect();
        }

        ScopedClipRect(const ScopedClipRect&) = delete;
        ScopedClipRect& operator=(const ScopedClipRect&) = delete;

    private:
        void PushClipRect(const Rect& rect)
        {
            m_renderTarget->PushAxisAlignedClip(
                TypeConversion::ToD2DRect(rect),
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
            );
            m_clipPushed = true;
        }

        void PopClipRect() noexcept
        {
            if (Detail::ShouldRevertOperation(m_renderTarget, m_clipPushed)) {
                m_renderTarget->PopAxisAlignedClip();
            }
        }

        ID2D1RenderTarget* m_renderTarget;
        bool m_clipPushed;
    };

} // namespace Spectrum::Helpers::Scopes

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_D2D_SCOPES_H